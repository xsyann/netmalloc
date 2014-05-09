/*
** syscall.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 12:11:26 2014 xsyann
** Last update Fri May  9 12:18:02 2014 xsyann
*/

#ifndef         __SYSCALL_H__
#define         __SYSCALL_H__

#include <linux/syscalls.h>

#define SYSCALL_NI __NR_tuxcall

void replace_syscall(ulong offset, ulong new_syscall);
void restore_syscall(ulong offset);

#endif          /* __SYSCALL_H__ */
