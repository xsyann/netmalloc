/*
** netmalloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:27 2014 xsyann
** Last update Fri May  9 12:21:22 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include "syscall.h"
#include "netmalloc.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(NETMALLOC_AUTHOR);
MODULE_DESCRIPTION(NETMALLOC_DESC);
MODULE_VERSION(NETMALLOC_VERSION);


static void *netmalloc_syscall(unsigned long size)
{
        printk("NetMalloc syscall : %ld\n", size);
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