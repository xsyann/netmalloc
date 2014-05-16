/*
** memory.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May 16 18:00:15 2014 xsyann
** Last update Fri May 16 18:28:14 2014 xsyann
*/

#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>

#include "netmalloc.h"
#include "memory.h"

static unsigned long get_last_vma_end(struct mm_struct *mm)
{
        struct vm_area_struct *area;
        unsigned long end = 0;

        for (area = mm->mmap; area; area = area->vm_next) {
                end = area->vm_end;
                PR_INFO("%p-%p", (void *)area->vm_start, (void *)area->vm_end);
        }
        return end;
}

static int create_vm_area(struct task_struct *task, struct vm_area_struct **vma,
                          struct vm_operations_struct *vm_ops,
                          unsigned long size)
{
        *vma = kzalloc(sizeof(**vma), GFP_KERNEL);
        if (*vma == NULL)
                return -ENOMEM;
        down_read(&task->mm->mmap_sem);
        INIT_LIST_HEAD(&(*vma)->anon_vma_chain);
        (*vma)->vm_mm = task->mm;
        (*vma)->vm_start = get_last_vma_end(current->mm);
        (*vma)->vm_end = (*vma)->vm_start + size;
        PR_INFO("start vma : %p, end vma : %p", (void*)(*vma)->vm_start, (void*)(*vma)->vm_end);
        (*vma)->vm_flags = VM_READ | VM_WRITE | VM_SPECIAL;
        (*vma)->vm_ops = vm_ops;
        up_read(&task->mm->mmap_sem);
        return 0;
}


struct vm_area_struct *add_vma(struct task_struct *task,
                               struct vm_operations_struct *vm_ops)
{
        struct vm_area_struct *vma;
        ulong* insert_vm_struct;

        insert_vm_struct = (ulong*)kallsyms_lookup_name("insert_vm_struct");
        if (insert_vm_struct == NULL) {
                PR_WARNING(NETMALLOC_WARN_SYMNOTFOUND);
                goto error;
        }
        if (create_vm_area(task, &vma, vm_ops, PAGE_SIZE) < 0)
                goto error;
        down_write(&current->mm->mmap_sem);
        if (((insert_vm_prot *)insert_vm_struct)(current->mm, vma)) {
                up_write(&current->mm->mmap_sem);
                kfree(vma);
                goto error;
        }
        up_write(&current->mm->mmap_sem);
        return vma;

error:
        return NULL;
}

