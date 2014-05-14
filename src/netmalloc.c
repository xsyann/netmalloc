/*
** netmalloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:27 2014 xsyann
** Last update Wed May 14 16:50:33 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "syscall.h"
#include "netmalloc.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(NETMALLOC_AUTHOR);
MODULE_DESCRIPTION(NETMALLOC_DESC);
MODULE_VERSION(NETMALLOC_VERSION);

static void netmalloc_vm_open(struct vm_area_struct *area)
{
        PR_INFO("open");
}

static void netmalloc_vm_close(struct vm_area_struct *area)
{
        PR_INFO("open");
}

static int netmalloc_vm_fault(struct vm_area_struct *area, struct vm_fault *vmf)
{
        PR_INFO("fault");
        return 0;
}

static struct vm_operations_struct netmalloc_vm_ops = {
        .open = netmalloc_vm_open,
        .close = netmalloc_vm_close,
        .fault = netmalloc_vm_fault
};

static int create_vm_area(struct vm_area_struct **vma)
{
        *vma = kmalloc(sizeof(**vma), GFP_KERNEL);
        if (*vma == NULL)
                return -ENOMEM;
        INIT_LIST_HEAD(&(*vma)->anon_vma_chain);
        (*vma)->vm_mm = current->mm;
/*      (*vma)->vm_start =
        (*vma)->vm_end = */
        (*vma)->vm_flags = VM_READ | VM_WRITE | VM_EXEC;
        (*vma)->vm_ops = &netmalloc_vm_ops;
        return 0;
}

static void *netmalloc_syscall(unsigned long size)
{
        struct vm_area_struct *vma;

        PR_INFO("called with a size of : %ld\n", size);
        PR_INFO("current process: %s, PID: %d", current->comm, current->pid);

        if (current->mm)
        {
                long start = current->mm->mmap->vm_start;
                long end = current->mm->mmap->vm_end;
                PR_INFO("start : %ld, end : %ld", start, end);
        } else
                PR_INFO("mm NULL");

        if (create_vm_area(&vma) < 0)
                return NULL;
        /* insert_vm_struct(current, vma);*/
        return NULL;
}

/* ************************************************************ */

static int __init netmalloc_init_module(void)
{
        PR_INFO(NETMALLOC_INFO_LOAD);
        replace_syscall(SYSCALL_NI, (ulong)netmalloc_syscall);
        return 0;
}

static void __exit netmalloc_exit_module(void)
{
        PR_INFO(NETMALLOC_INFO_UNLOAD);
        restore_syscall(SYSCALL_NI);
}

module_init(netmalloc_init_module);
module_exit(netmalloc_exit_module);
