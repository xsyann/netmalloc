/*
** area.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Tue May 20 04:14:57 2014 xsyann
** Last update Thu May 22 21:36:32 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched.h>
#include "kutils.h"
#include "vma.h"
#include "generic_malloc.h"
#include "netmalloc.h"
#include "area.h"

/* Return region at address in area. */
static struct region_struct *get_region(struct area_struct *area,
                                        unsigned long address)
{
        struct region_struct *region;

        list_for_each_entry(region, &area->regions.list, list)
                if ((unsigned long)region->virtual_start == address)
                        return region;
        return NULL;
}

/* Return area containing address for pid */
static struct area_struct *get_area(pid_t pid, unsigned long address,
                             struct area_struct *area_list)
{
        struct area_struct *area;

        list_for_each_entry(area, &area_list->list, list)
                if (area->pid == pid)
                        if (address >= area->vma->vm_start &&
                            address < area->vma->vm_end)
                                return area;
        return NULL;
}

/* Create new area and insert it in area_list. */
static struct area_struct *add_area_struct(unsigned long size, pid_t pid,
                                           struct area_struct *area_list)
{
        struct area_struct *area;

        area = kmalloc(sizeof(*area), GFP_KERNEL);
        area->size = size;
        area->pid = pid;
        area->free_space = size;
        INIT_LIST_HEAD(&area->list);
        INIT_LIST_HEAD(&area->regions.list);
        list_add_tail(&area->list, &area_list->list);
        return area;
}

/* Add a new region after prev in area. */
static struct region_struct *add_region_after(struct region_struct *prev,
                                              struct area_struct *area,
                                              unsigned long size)
{
        struct region_struct *region;

        region = kmalloc(sizeof(*region), GFP_KERNEL);
        if (region == NULL)
                return NULL;
        region->size = size;
        region->free = 0;
        area->free_space -= size;
        INIT_LIST_HEAD(&region->list);
        list_add(&region->list, &prev->list);

        /* Set virtual start */
        if (&prev->list == &area->regions.list)
                region->virtual_start = (void *)area->vma->vm_start;
        else
                region->virtual_start = prev->virtual_start + prev->size;
        return region;
}

/* Merge region with next or prev if they are free.
 * Remove region if last in vma.
*/
void merge_neighbors_region(struct region_struct *region,
                            struct region_struct *first)
{
        struct region_struct *next;
        struct region_struct *prev;

        next = list_entry(region->list.next, struct region_struct, list);

        /* Merge with next */
        if (next != first && next->free == 1) {
                region->size += next->size;
                list_del(&next->list);
                kfree(next);
        }

        /* Merge with prev */
        prev = list_entry(region->list.prev, struct region_struct, list);
        if (region != first && prev->free == 1) {
                prev->size += region->size;
                list_del(&region->list);
                kfree(region);
                region = prev;
        }

        /* Remove if last */
        next = list_entry(region->list.next, struct region_struct, list);
        if (next == first) {
                list_del(&region->list);
                kfree(region);
        }
}

/* Return a region of 'size' size. Return NULL if no region available. */
static struct region_struct *get_free_region(struct area_struct *area,
                                             unsigned long size)
{
        struct region_struct *region;
        struct region_struct *split;
        unsigned long region_size;

        list_for_each_entry(region, &area->regions.list, list) {
                if (region->size == size && region->free == 1) {
                        region->free = 0;
                        return region;
                } /* Split the region if too big */
                else if (region->size > size && region->free == 1) {
                        region_size = region->size;
                        region->free = 0;
                        region->size = size;
                        area->free_space -= size;

                        split = add_region_after(region, area, region_size - size);
                        if (split == NULL)
                                return NULL;
                        split->free = 1;
                        area->free_space += split->size;
                        return region;
                }
        }
        return NULL;
}

/* Use a free region if possible or create a new one at list end. */
static struct region_struct *add_region_struct(struct area_struct *area,
                                               unsigned long size)
{
        struct region_struct *region;

        region = get_free_region(area, size);
        if (region)
                return region;
        region = add_region_after(list_entry(area->regions.list.prev,
                                             struct region_struct, list),
                                  area, size);
        return region;
}

/* Align size (size >= alignment, size % alignment == 0). */
static unsigned long align_size(unsigned long size, unsigned long alignment)
{
        size = size < alignment ? alignment : size;
        if (size % alignment && size > alignment)
                size = size + alignment - (size % alignment);
        return size;
}

/* Extend vma if vma is last.
 * Return 0 on success and negative on error. */
static int extend_vma(struct area_struct *area, unsigned long size)
{
        unsigned long extra;

        if (is_last_vma(area->vma))
        {
                extra = align_size(area->size + size, PAGE_SIZE) - area->size;
                area->vma->vm_end += extra;
                area->size += extra;
                area->free_space += extra;
                return 0;
        }
        return -1;
}

/* Return the first area_struct with 'size' space free or create one. */
static struct area_struct *get_vma_container(unsigned long size,
                                             struct task_struct *task,
                                             struct area_struct *area_list,
                                             struct vm_operations_struct *vm_ops)
{
        struct area_struct *area;

        list_for_each_entry(area, &area_list->list, list) {
                if (area->pid == task->pid) {
                        if (area->free_space >= size) /* Enough free space */
                                return area;
                        else if (extend_vma(area, size) == 0) /* Extend vma */
                                return area;
                }
        }
        /* Create new vma */
        size = align_size(size, VMA_SIZE);

        area = add_area_struct(size, task->pid, area_list);

        area->vma = add_vma(task, vm_ops, size);
        if (area->vma == NULL) {
                PR_WARN(NETMALLOC_WARN_VMA);
                return NULL;
        }
        return area;
}

/* ************************************************************ */

struct region_struct *create_region(unsigned long size,
                                    struct task_struct *task,
                                    struct area_struct *area_list,
                                    struct vm_operations_struct *vm_ops)
{
        struct region_struct *region;
        struct area_struct *area = get_vma_container(size, task,
                                                     area_list, vm_ops);
        if (area == NULL)
                return NULL;
        region = add_region_struct(area, size);
        return region;
}

void remove_region(pid_t pid, unsigned long address,
                   struct area_struct *area_list)
{
        struct area_struct *area;
        struct region_struct *region;

        area = get_area(pid, address, area_list);
        if (area) {
                region = get_region(area, address);
                if (region) {
                        area->free_space += region->size;
                        region->free = 1;
                        /* Merge with neighbors */
                        merge_neighbors_region(region, &area->regions);

                        /* Remove Area if empty */
                        if (area->regions.list.next == area->regions.list.prev)
                                remove_vma(area->vma);
                }
        }
}

void init_area_list(struct area_struct *area_list)
{
        INIT_LIST_HEAD(&area_list->list);
}

void remove_vma_from_area_list(struct area_struct *area_list,
                               struct vm_area_struct *vma)
{
        struct area_struct *area, *tmp_area;
        struct region_struct *region, *tmp_region;

        list_for_each_entry_safe(area, tmp_area, &area_list->list, list) {
                if (area->vma == vma) {
                        list_for_each_entry_safe(region, tmp_region,
                                                 &area->regions.list, list) {
                                list_del(&region->list);
                                kfree(region);
                        }
                        list_del(&area->list);
                        kfree(area);
                }
        }
}

void remove_area_list(struct area_struct *area_list)
{
        struct area_struct *area, *tmp_area;
        struct region_struct *region, *tmp_region;

        list_for_each_entry_safe(area, tmp_area, &area_list->list, list) {
                list_for_each_entry_safe(region, tmp_region,
                                         &area->regions.list, list) {
                        list_del(&region->list);
                        kfree(region);
                }
                list_del(&area->list);
                kfree(area);
        }
}
