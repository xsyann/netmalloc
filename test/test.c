/*
** test.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:46:35 2014 xsyann
** Last update Fri May  9 12:22:03 2014 xsyann
*/

#include <sys/syscall.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define NETMALLOC_SYSCALL __NR_tuxcall

void *netmalloc(unsigned long size)
{
        return (void *)syscall(NETMALLOC_SYSCALL, size);
}

int main(void)
{
        int size = 2048;
        printf("Netmalloc return %p\n", netmalloc(size));
        return 0;
}
