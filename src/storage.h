/*
** storage.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Wed May 21 11:22:40 2014 xsyann
** Last update Fri May 23 05:27:45 2014 xsyann
*/

#ifndef         __STORAGE_H__
#define         __STORAGE_H__

struct storage_ops
{
        int (*save)(pid_t pid, unsigned long address, void *buffer);
        int (*load)(pid_t pid, unsigned long address, void *buffer);
        void (*remove)(pid_t pid, unsigned long address);
        void (*release)(void);
};

struct kernel_buffer_list
{
        pid_t pid;
        unsigned long address;
        void *buffer;
        struct list_head list;
};

int kernel_load(pid_t pid, unsigned long address, void *buffer);
int kernel_save(pid_t pid, unsigned long address, void *buffer);
void kernel_remove(pid_t pid, unsigned long address);
void kernel_release(void);

#endif          /* __STORAGE_H__ */
