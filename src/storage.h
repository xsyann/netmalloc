/*
** storage.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Wed May 21 11:22:40 2014 xsyann
** Last update Wed May 21 11:25:13 2014 xsyann
*/

#ifndef         __STORAGE_H__
#define         __STORAGE_H__

struct storage_ops
{
        int (*save)(unsigned long address, void *buffer);
        int (*load)(unsigned long address, void *buffer);
        void (*release)(void);
};

struct kernel_buffer_list
{
        unsigned long address;
        void *buffer;
        struct list_head list;
};

int kernel_load(unsigned long address, void *buffer);
int kernel_save(unsigned long address, void *buffer);
void kernel_release(void);

#endif          /* __STORAGE_H__ */
