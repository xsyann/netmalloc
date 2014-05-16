/*
** test.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:46:35 2014 xsyann
** Last update Fri May 16 22:18:47 2014 xsyann
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
        static int foo = 1;
        int size = 2048;
        void *heap = malloc(1);

        printf("Static var: %p\n", &foo);
        printf("Heap var:   %p\n", heap);
        printf("Stack var:  %p\n", &size);
        free(heap);
        printf("Netmalloc return %p\n", netmalloc(size));
/*        int *t = 0x3ff000;
          t[0] = 1; */
/*        while (1); */
        return 0;
}
