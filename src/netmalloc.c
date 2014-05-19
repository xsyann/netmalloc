/*
** netmalloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:27 2014 xsyann
** Last update Sat May 17 15:39:56 2014 xsyann
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

static void *buffer = NULL;

static void netmalloc_vm_open(struct vm_area_struct *vma)
{
        PR_INFO("open");
}

static void netmalloc_vm_close(struct vm_area_struct *vma)
{
        PR_INFO("close");
}

static void *get_buffer(void)
{
        if (buffer == NULL)
                buffer = vmalloc_user(PAGE_SIZE);
        return buffer;
}

static struct page *netmalloc_get_page(unsigned long offset)
{
        struct page *page;
        void *page_address;

        if ((page_address = get_buffer()) == NULL)
                return NULL;
        page = vmalloc_to_page(page_address);
        return page;
}

static int netmalloc_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
        unsigned long address = (unsigned long)(vmf->virtual_address);
        unsigned long offset = address - vma->vm_start;
        struct page *page;

        PR_INFO("fault at %p - offset %p", (void *)address, (void *)offset);

        page = netmalloc_get_page(offset);
        if (page == NULL) {
                PR_WARNING(NETMALLOC_WARN_PAGE);
                return VM_FAULT_SIGBUS;
        }
        vm_insert_page(vma, address, page);
        vmf->page = page;
        return 0;
}

static struct vm_operations_struct netmalloc_vm_ops = {
        .open = netmalloc_vm_open,
        .close = netmalloc_vm_close,
        .fault = netmalloc_vm_fault
};

static void *netmalloc_syscall(unsigned long size)
{
        static struct vm_area_struct *vma;

        PR_INFO("Called with a size of : %ld\n", size);
        PR_INFO("Current process: %s, PID: %d", current->comm, current->pid);

        vma = add_vma(current, &netmalloc_vm_ops);
        if (vma == NULL) {
                PR_WARNING(NETMALLOC_WARN_VMA);
                return NULL;
        }
        return (void *)vma->vm_start;
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
        if (buffer != NULL)
                vfree(buffer);
        PR_INFO(NETMALLOC_INFO_UNLOAD);
        restore_syscall(SYSCALL_NI);
}

module_init(netmalloc_init_module);
module_exit(netmalloc_exit_module);
