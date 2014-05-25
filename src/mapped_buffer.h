/*
** mapped_buffer.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sat May 24 01:46:14 2014 xsyann
** Last update Sun May 25 09:00:49 2014 xsyann
*/

#ifndef         __MAPPED_BUFFER_H__
#define         __MAPPED_BUFFER_H__

struct mapped_buffer
{
        void *buffer;
        struct vm_area_struct *vma;
        unsigned long start;
        pid_t pid;

        void *cache;
        struct vm_area_struct *cache_vma;
        unsigned long cache_start;

        struct list_head list;
};

struct mapped_buffer *get_buffer(pid_t pid, struct mapped_buffer *buffers);
void swap_buffer_cache(struct mapped_buffer *buffer);
void remove_buffer(pid_t pid, struct mapped_buffer *buffers);
struct mapped_buffer *add_buffer(pid_t pid, struct mapped_buffer *buffers);
void init_buffers(struct mapped_buffer *buffers);
void remove_buffers(struct mapped_buffer *buffers);

void dump_buffers(struct mapped_buffer *buffers);

#endif          /* __MAPPED_BUFFER_H__ */
