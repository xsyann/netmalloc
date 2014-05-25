/*
** netmalloc.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:53 2014 xsyann
** Last update Sun May 25 06:42:22 2014 xsyann
*/

#ifndef         __NETMALLOC_H__
#define         __NETMALLOC_H__

#define NETMALLOC_AUTHOR "Nicolas de Thore, Yann Koeth"
#define NETMALLOC_DESC "Memory allocation over network"
#define NETMALLOC_VERSION "0.1"

#define NETMALLOC_INFO_LOAD "Loaded"
#define NETMALLOC_INFO_UNLOAD "Unloaded"

#define NETMALLOC_WARN_SYMNOTFOUND "insert_vm_struct symbol not found"
#define NETMALLOC_WARN_VMA "Unable to create vm area"
#define NETMALLOC_WARN_PAGE "Unable to get page %016lx"
#define NETMALLOC_WARN_STORAGE "Unable to init storage"

#endif          /* __NETMALLOC_H__ */
