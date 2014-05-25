/*
** netmalloc.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sun May 25 02:15:08 2014 xsyann
** Last update Sun May 25 02:25:13 2014 xsyann
*/

#include <sys/syscall.h>
#include <stdint.h>

#define NETMALLOC_SYSCALL __NR_tuxcall
#define NETFREE_SYSCALL __NR_security

static inline void *netmalloc(unsigned long size)
{
        return (void *)((intptr_t)syscall(NETMALLOC_SYSCALL, size));
}

static inline void netfree(void *ptr)
{
        syscall(NETFREE_SYSCALL, ptr);
}
