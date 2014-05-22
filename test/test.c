/*
** test.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:46:35 2014 xsyann
** Last update Thu May 22 20:18:43 2014 xsyann
*/

#include <sys/syscall.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define NETMALLOC_SYSCALL __NR_tuxcall
#define NETFREE_SYSCALL __NR_security
#define PAGE_SIZE (1 << 12)

void *netmalloc(unsigned long size)
{
        return (void *)syscall(NETMALLOC_SYSCALL, size);
}

void netfree(void *ptr)
{
        syscall(NETFREE_SYSCALL, ptr);
}

void dump_buffer(char *buffer, size_t size, const char *page_marker)
{
        size_t i;

        for (i = 0; i < size; ++i) {
                if (i % 30 == 0)
                        printf("\n");
                if (i % PAGE_SIZE == 0)
                        printf("[%s]", page_marker);
                printf("%c ", buffer[i]);
        }
        printf("\n");
}

void *thread1_main(void *param)
{
        (void)param;
        printf("Thread1\n");
        char *buffer = netmalloc(5000);
        memset(buffer, 'A', 1000);
        memset(buffer+1000, 'a', 4000);
        sprintf(buffer + 4500, "hello %d %s", 42, "thread 1");
        dump_buffer(buffer, 5000, "t1");
        netfree(buffer);
        return NULL;
}

void *thread2_main(void *param)
{
        (void)param;
        printf("Thread2\n");
        char *buffer = netmalloc(5000);
        memset(buffer, 'B', 1000);
        memset(buffer+1000, 'b', 4000);
        sprintf(buffer + 4500, "foo %d %s", 18, "thread 2 string");
        dump_buffer(buffer, 5000, "t2");
        netfree(buffer);
        return NULL;
}

int main(void)
{

        pthread_t thread1, thread2, thread3, thread4;

        if (pthread_create(&thread1, NULL, thread1_main, NULL))
                return 1;
        if (pthread_create(&thread2, NULL, thread2_main, NULL))
                return 1;
        if (pthread_create(&thread3, NULL, thread1_main, NULL))
                return 1;
        if (pthread_create(&thread4, NULL, thread2_main, NULL))
                return 1;
        if (pthread_join(thread1, NULL))
                return 2;
        if (pthread_join(thread2, NULL))
                return 2;
        if (pthread_join(thread3, NULL))
                return 2;
        if (pthread_join(thread4, NULL))
                return 2;
        char *titi = netmalloc(2);
        netfree(titi);

        char *buffer = netmalloc(64);
        char *buffer1 = netmalloc(512);
        char *buffer2 = netmalloc(1024);
//        netfree(buffer1);
        buffer1 = netmalloc(510);
        netfree(buffer2);
        //       netfree(buffer);
        netfree(buffer1);

//        usleep(300);

        printf("Main\n");
        buffer1 = netmalloc(512);
        memset(buffer1, 'B', 512);
        sprintf(buffer1 + 100, "hellom %d %s", 18, "main");
        dump_buffer(buffer1, 512, "m");
        dump_buffer(buffer1, 512, "m");
        dump_buffer(buffer1, 512, "m");
        dump_buffer(buffer1, 512, "m");
//        netfree(buffer1);

        static int foo = 1;
        int size = 5102;
        void *heap = malloc(1);

        printf("Static var: %p\n", &foo);
        printf("Heap var:   %p\n", heap);
        printf("Stack var:  %p\n", &size);
        free(heap);
        buffer = netmalloc(size);
        printf("Netmalloc return %p\n", buffer);
        buffer[2] = 42;
        buffer[12] = 255;
        buffer[4098] = 18;
        dump_buffer(buffer, size, "$");
        buffer[42] = 1;
        buffer[4099] = 2;
        dump_buffer(buffer, size, "$");

        buffer1 = netmalloc(512);
        buffer1[1] = 2;
        dump_buffer(buffer1, 512, "$");

        buffer2 = netmalloc(15120);
        memset(buffer2, 'B', 5120);
        sprintf(buffer2, "tutu %d", 22);
        sprintf(buffer2 + 4090, "tata %d %s", 18, "barfoo");
        sprintf(buffer2 + 14090, "hello %d %s", 42, "foobar");
        dump_buffer(buffer2, 15120, "$");

        //  netfree(buffer);
        netfree(buffer1);
        //     netfree(buffer2);

        int *ptr;

        ptr = netmalloc(sizeof(int));
        if (ptr == NULL)
                return 1;
        *ptr = 25;
        printf("%d\n", *ptr);
//        netfree(ptr);


        return 0;
}
