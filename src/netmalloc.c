/*
** netmalloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:27 2014 xsyann
** Last update Fri May 16 22:17:45 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "syscall.h"
#include "memory.h"
#include "netmalloc.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(NETMALLOC_AUTHOR);
MODULE_DESCRIPTION(NETMALLOC_DESC);
MODULE_VERSION(NETMALLOC_VERSION);

static struct vm_area_struct *vma; /* Handle one vma */

static void netmalloc_vm_open(struct vm_area_struct *area)
{
        PR_INFO("open");
}

static void netmalloc_vm_close(struct vm_area_struct *area)
{
        PR_INFO("close");
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

static void *netmalloc_syscall(unsigned long size)
{

        PR_INFO("Called with a size of : %ld\n", size);
        PR_INFO("Current process: %s, PID: %d", current->comm, current->pid);

        vma = add_vma(current, &netmalloc_vm_ops);
        if (vma == NULL) {
                PR_WARNING(NETMALLOC_WARN_VMA);
                return NULL;
        }
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
