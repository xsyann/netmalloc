/*
** test.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:46:35 2014 xsyann
** Last update Tue May 20 06:02:55 2014 xsyann
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

void dump_buffer(char *buffer, size_t size)
{
        size_t i;

        for (i = 0; i < size; ++i) {
                if (i % 30 == 0)
                        printf("\n");
                printf("%d ", buffer[i]);
        }
        printf("\n");
}

int main(void)
{
        static int foo = 1;
        int size = 5102;
        void *heap = malloc(1);

        printf("Static var: %p\n", &foo);
        printf("Heap var:   %p\n", heap);
        printf("Stack var:  %p\n", &size);
        free(heap);
        char *buffer = netmalloc(size);
        printf("Netmalloc return %p\n", buffer);
        buffer[2] = 42;
        buffer[12] = 255;
        buffer[4098] = 18;
        dump_buffer(buffer, size);
        buffer[42] = 1;
        buffer[4099] = 2;
        dump_buffer(buffer, size);

        char *buffer1 = netmalloc(512);
        buffer1[1] = 2;
        dump_buffer(buffer1, 512);
/*        while (1); */
        return 0;
}
