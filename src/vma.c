/*
** vma.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May 16 18:00:15 2014 xsyann
** Last update Thu May 22 21:38:17 2014 xsyann
*/

#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>

#include "netmalloc.h"
#include "kutils.h"
#include "vma.h"

static unsigned long get_free_space(struct mm_struct *mm, unsigned long size)
{
        struct vm_area_struct *area;
        unsigned long end = PAGE_SIZE;

        for (area = mm->mmap; area; area = area->vm_next) {
                if (area->vm_start - end >= size) {
                        return end;
                }
                end = area->vm_end;
                PR_DEBUG(D_MED, "Existing vma at %016lx-%016lx",
                         area->vm_start, area->vm_end);
        }
        return end;
}

static int create_vm_area_struct(struct task_struct *task,
                                 struct vm_area_struct **vma,
                                 struct vm_operations_struct *vm_ops,
                                 unsigned long size)
{
        *vma = kzalloc(sizeof(**vma), GFP_KERNEL);
        if (*vma == NULL)
                return -ENOMEM;
        INIT_LIST_HEAD(&(*vma)->anon_vma_chain);
        (*vma)->vm_mm = task->mm;
        (*vma)->vm_start = get_free_space(current->mm, size);
        (*vma)->vm_end = (*vma)->vm_start + size;
        PR_DEBUG(D_MED, "Creating vma at %016lx-%016lx",
                 (*vma)->vm_start, (*vma)->vm_end);
        (*vma)->vm_flags = VM_READ | VM_WRITE | VM_EXEC | VM_MIXEDMAP;
        (*vma)->vm_page_prot = vm_get_page_prot((*vma)->vm_flags);
        (*vma)->vm_ops = vm_ops;
        return 0;
}

int is_last_vma(struct vm_area_struct *vma)
{
        return vma->vm_next == NULL;
}

int is_existing_vma(struct vm_area_struct *vma)
{
       struct vm_area_struct *area;

       for (area = vma->vm_mm->mmap; area; area = area->vm_next)
               if (vma == area)
                       return 1;
       return 0;
}

void remove_vma(struct vm_area_struct *vma)
{
        ulong* do_munmap;

        do_munmap = (ulong*)kallsyms_lookup_name("do_munmap");
        if (do_munmap) {
                ((do_munmap_prot *)do_munmap)(vma->vm_mm, vma->vm_start,
                                              vma->vm_end - vma->vm_start);
        }
}

struct vm_area_struct *add_vma(struct task_struct *task,
                               struct vm_operations_struct *vm_ops,
                               unsigned long size)
{
        struct vm_area_struct *vma;
        ulong* insert_vm_struct;

        insert_vm_struct = (ulong*)kallsyms_lookup_name("insert_vm_struct");
        if (insert_vm_struct == NULL) {
                PR_WARN(NETMALLOC_WARN_SYMNOTFOUND);
                goto error;
        }

        if (create_vm_area_struct(task, &vma, vm_ops, size) < 0)
                goto error;

        if (((insert_vm_prot *)insert_vm_struct)(current->mm, vma)) {
                kfree(vma);
                goto error;
        }

        return vma;

error:
        return NULL;
}

