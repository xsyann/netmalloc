/*
** test1.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sun May 25 02:14:44 2014 xsyann
** Last update Sun May 25 08:56:46 2014 xsyann
*/

#include <stdio.h>
#include "netmalloc.h"

char *my_strcpy(char *s1, const char *s2)
{
        char *s = s1;
        char c;
        while (*s2 != 0) {
                c = *s2;
                *s = c;
                *s++;
                *s2++;
                //*s++ = *s2++;
        }
        //while ((*s++ = *s2++) != 0);
        return (s1);
}

int main(void)
{
        char *buffer = netmalloc(5120);
        sprintf(buffer, "toto");
        sprintf(buffer + 4094, "foobar"); //-> BUG
//        strcpy(buffer + 4094, "foobar"); //-> BUG
//        my_strcpy(buffer + 4094, "foobar");
//        my_strcpy(buffer + 14094, "fiibee");
        printf("toto = %s\n", buffer);
        printf("foobar = %s\n", buffer + 4094);
//        printf("fiibee = %s\n", buffer + 14094);
        netfree(buffer);
        return 0;
}
