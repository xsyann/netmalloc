/*
** netmalloc.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:53 2014 xsyann
** Last update Fri May  9 11:54:28 2014 xsyann
*/

#ifndef         __NETMALLOC_H__
#define         __NETMALLOC_H__

#define NETMALLOC_AUTHOR "Nicolas de Thore, Yann Koeth"
#define NETMALLOC_DESC "Memory allocation over network"
#define NETMALLOC_VERSION "0.1"

#define NETMALLOC_MODNAME KBUILD_MODNAME
#define NETMALLOC_INFO_LOAD "Loaded"
#define NETMALLOC_INFO_UNLOAD "Unloaded"

#define PR_WARNING(ERR, ...) \
        (printk(KERN_WARNING "%s: error: " ERR "\n", NETMALLOC_MODNAME, ##__VA_ARGS__))

#define PR_INFO(INFO, ...) \
        (printk(KERN_INFO "%s: " INFO "\n", NETMALLOC_MODNAME, ##__VA_ARGS__))


#endif          /* __NETMALLOC_H__ */
