/*
** netmalloc.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:53 2014 xsyann
** Last update Tue May 20 04:57:51 2014 xsyann
*/

#ifndef         __NETMALLOC_H__
#define         __NETMALLOC_H__

#define NETMALLOC_DEBUG 1

#define VMA_SIZE PAGE_SIZE * 2

#define NETMALLOC_AUTHOR "Nicolas de Thore, Yann Koeth"
#define NETMALLOC_DESC "Memory allocation over network"
#define NETMALLOC_VERSION "0.1"

#define NETMALLOC_MODNAME KBUILD_MODNAME
#define NETMALLOC_INFO_LOAD "Loaded"
#define NETMALLOC_INFO_UNLOAD "Unloaded"

#define NETMALLOC_WARN_SYMNOTFOUND "insert_vm_struct symbol not found"
#define NETMALLOC_WARN_VMA "Unable to create vm area"
#define NETMALLOC_WARN_PAGE "Unable to get a memory page"

#define PR_WARN(ERR, ...) \
        (printk(KERN_WARNING "%s: error: " ERR "\n", NETMALLOC_MODNAME, ##__VA_ARGS__))

#define PR_INFO(INFO, ...) \
        (printk(KERN_INFO "%s: " INFO "\n", NETMALLOC_MODNAME, ##__VA_ARGS__))

#ifdef NETMALLOC_DEBUG
#define PR_DEBUG(INFO, ...) \
        (printk(KERN_INFO "%s: " INFO "\n", NETMALLOC_MODNAME, ##__VA_ARGS__))
#else
#define PR_DEBUG(INFO, ...)
#endif

#endif          /* __NETMALLOC_H__ */
