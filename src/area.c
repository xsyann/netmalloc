/*
** area.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Tue May 20 04:14:57 2014 xsyann
** Last update Wed May 21 17:26:49 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched.h>
#include "netmalloc.h"
#include "vma.h"
#include "area.h"

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

static struct region_struct *add_region_struct(struct area_struct *area,
                                               unsigned long size)
{
        struct region_struct *region;
        struct region_struct *prev;

        region = kmalloc(sizeof(*region), GFP_KERNEL);

        region->size = size;
        region->virtual_start = (void *)area->vma->vm_start;
        area->free_space -= size;
        INIT_LIST_HEAD(&region->list);
        list_add_tail(&region->list, &area->regions.list);

        /* Set virtual_start */
        if (region->list.prev != &area->regions.list) {
                prev = list_entry(region->list.prev,
                                  struct region_struct, list);
                region->virtual_start = prev->virtual_start + prev->size;
        }
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

/* Return 0 on success and negative on error. */
static int extend_vma(struct area_struct *area, unsigned long size)
{
        unsigned long extra;

        down_write(&area->vma->vm_mm->mmap_sem);
        if (is_last_vma(area->vma))
        {
                extra = align_size(area->size + size, PAGE_SIZE) - area->size;
                area->vma->vm_end += extra;
                area->size += extra;
                area->free_space += extra;
                up_write(&area->vma->vm_mm->mmap_sem);
                return 0;
        }
        up_write(&area->vma->vm_mm->mmap_sem);
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
