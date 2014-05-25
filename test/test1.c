/*
** test1.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sun May 25 02:14:44 2014 xsyann
** Last update Sun May 25 02:16:13 2014 xsyann
*/

#include <stdio.h>
#include "netmalloc.h"

typedef struct rec
{
        int i;
        float PI;
        char A;
}RECORD;

int main()
{
        RECORD *ptr_one;

        ptr_one = (RECORD *) netmalloc (sizeof(RECORD));

        (*ptr_one).i = 10;
        (*ptr_one).PI = 3.14;
        (*ptr_one).A = 'a';

        printf("First value: %d\n",(*ptr_one).i);
        printf("Second value: %f\n", (*ptr_one).PI);
        printf("Third value: %c\n", (*ptr_one).A);

        netfree(ptr_one);

        return 0;
}
