/*
** netmalloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:27 2014 xsyann
** Last update Sun May 25 07:09:26 2014 xsyann
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

MODULE_AUTHOR(NETMALLOC_AUTHOR);
MODULE_DESCRIPTION(NETMALLOC_DESC);
MODULE_VERSION(NETMALLOC_VERSION);
MODULE_LICENSE("GPL");

static char *server = "127.0.0.1:12345";

module_param(server, charp, 0);
MODULE_PARM_DESC(server, "host:port");

/* ************************************************************ */

static struct storage_ops storage_ops = {
        .init = network_init,
        .save = network_save,
        .load = network_load,
        .remove = network_remove,
        .release = network_release
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

        return generic_malloc_init(&storage_ops, server);
}

static void __exit netmalloc_exit_module(void)
{
        restore_syscall(SYSCALL_NI2);
        restore_syscall(SYSCALL_NI1);

        generic_malloc_clean();
        PR_INFO(NETMALLOC_INFO_UNLOAD);
}

module_init(netmalloc_init_module);
module_exit(netmalloc_exit_module);
