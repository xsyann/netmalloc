/*
** netmalloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:27 2014 xsyann
** Last update Thu May 22 21:16:04 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include "syscall.h"
#include "vma.h"
#include "area.h"
#include "storage.h"
#include "generic_malloc.h"
#include "kutils.h"
#include "netmalloc.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(NETMALLOC_AUTHOR);
MODULE_DESCRIPTION(NETMALLOC_DESC);
MODULE_VERSION(NETMALLOC_VERSION);

/* ************************************************************ */

static struct storage_ops storage_ops = {
        .save = kernel_save,
        .load = kernel_load,
        .remove = kernel_remove,
        .release = kernel_release
};

/* ************************************************************ */

static void netfree_syscall(void *ptr)
{
        generic_free(ptr);
}

static void *netmalloc_syscall(unsigned long size)
{
        return generic_malloc(size);
}

/* ************************************************************ */

static int __init netmalloc_init_module(void)
{
        PR_INFO(NETMALLOC_INFO_LOAD);
        replace_syscall(SYSCALL_NI1, (ulong)netmalloc_syscall);
        replace_syscall(SYSCALL_NI2, (ulong)netfree_syscall);

        generic_malloc_init(&storage_ops);

        return 0;
}

static void __exit netmalloc_exit_module(void)
{
        PR_INFO(NETMALLOC_INFO_UNLOAD);
        restore_syscall(SYSCALL_NI2);
        restore_syscall(SYSCALL_NI1);

        generic_malloc_clean();
}

module_init(netmalloc_init_module);
module_exit(netmalloc_exit_module);
