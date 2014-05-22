/*
** syscall.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 12:11:26 2014 xsyann
** Last update Wed May 21 18:24:47 2014 xsyann
*/

#ifndef         __SYSCALL_H__
#define         __SYSCALL_H__

#include <linux/syscalls.h>

#define SYSCALL_NI1 __NR_tuxcall
#define SYSCALL_NI2 __NR_security

void replace_syscall(ulong offset, ulong new_syscall);
void restore_syscall(ulong offset);

#endif          /* __SYSCALL_H__ */
