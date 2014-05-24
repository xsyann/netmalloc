/*
** area.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Tue May 20 04:14:57 2014 xsyann
** Last update Sat May 24 22:13:58 2014 xsyann
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
        if (list_is_first(&region->list, &area->regions.list))
                region->virtual_start = (void *)area->vma->vm_start;
        else
                region->virtual_start = prev->virtual_start + prev->size;
        return region;
}

/* Merge region with next or prev if they are free.
 * Remove region if last in vma.
*/
static void merge_neighbors_region(struct region_struct *region,
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
                        merge_neighbors_region(split, &area->regions);
                        return region;
                }
        }
        return NULL;
}

/* Use a free region if possible or create a new one at list end. */
static struct region_struct *add_region(struct area_struct *area,
                                               unsigned long size)
{
        struct region_struct *region;
        struct region_struct *last_region;

        region = get_free_region(area, size);
        if (region)
                return region;

        last_region = list_last_entry(&area->regions.list,
                                      struct region_struct, list);
        region = add_region_after(last_region, area, size);

        return region;
}

/* Extend vma if possible.
 * Return 0 on success and negative on error. */
static int extend_vma(struct area_struct *area, unsigned long size)
{
        unsigned long extra;

        extra = align_ceil(area->size + size - area->free_space, PAGE_SHIFT) - area->size;
        if (is_last_vma(area->vma) ||
            area->vma->vm_next->vm_start > area->vma->vm_end + extra)
        {
                area->vma->vm_end += extra;
                area->size += extra;
                area->free_space += extra;
                return 0;
        }
        return -1;
}

/* Return the first area_struct with 'size' space free or create one. */
static struct area_struct *get_area_container(unsigned long size,
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
        size = align_ceil(size, VMA_SHIFT);

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
        struct area_struct *area = get_area_container(size, task,
                                                     area_list, vm_ops);
        if (area == NULL)
                return NULL;
        region = add_region(area, size);
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

                        /* Remove vma if empty */
                        /* (area_struct will be deleted by close hander) */
                        if (list_empty(&area->regions.list))
                                remove_vma(area->vma);
                }
        }
}

/* Print all areas and regions */
void dump_area_list(struct area_struct *area_list)
{
        struct area_struct *area;
        struct region_struct *region;

        PR_DEBUG(D_MED, "");
        PR_DEBUG(D_MED, "Area list :");
        list_for_each_entry(area, &area_list->list, list) {
                PR_DEBUG(D_MED, "- VMA : size = %ld, pid = %d, freespace = %ld",
                        area->size, area->pid, area->free_space);
                list_for_each_entry(region, &area->regions.list, list) {
                        PR_DEBUG(D_MED, "   - Region, size = %ld, start = %p, free = %d",
                                 region->size, region->virtual_start, region->free);
                }
        }
              PR_DEBUG(D_MED, "");
}

/* Return 1 if address is a valid virtual address and 0 if not. */
int is_valid_address(unsigned long address, struct vm_area_struct *vma,
                     struct area_struct *area_list)
{
        struct area_struct *area;
        struct region_struct *region;
        unsigned long start, end;

        list_for_each_entry(area, &area_list->list, list) {
                if (area->vma == vma) {
                        list_for_each_entry(region, &area->regions.list, list) {
                                start = (unsigned long)region->virtual_start;
                                end = start + region->size;
                                /* Page align */
                                start = align_floor(start, PAGE_SHIFT);
                                end = align_ceil(end, PAGE_SHIFT);
                                if (address >= start && address < end &&
                                    region->free == 0)
                                        return 1;
                        }
                }
        }
        return 0;
}

/* Return region at address in area. */
struct region_struct *get_region(struct area_struct *area,
                                        unsigned long address)
{
        struct region_struct *region;

        list_for_each_entry(region, &area->regions.list, list)
                if ((unsigned long)region->virtual_start == address)
                        return region;
        return NULL;
}

/* Return area containing address for pid */
struct area_struct *get_area(pid_t pid, unsigned long address,
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

/* Return area count for pid */
int get_area_count(pid_t pid, struct area_struct *area_list)
{
        struct area_struct *area;
        int count = 0;

        list_for_each_entry(area, &area_list->list, list)
                if (area->pid == pid)
                        ++count;
        return count;
}

void init_area_list(struct area_struct *area_list)
{
        INIT_LIST_HEAD(&area_list->list);
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
