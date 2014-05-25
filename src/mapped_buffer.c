/*
** mapped_buffer.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sat May 24 01:49:48 2014 xsyann
** Last update Sun May 25 08:49:35 2014 xsyann
*/

#include <linux/list.h>
#include <linux/slab.h>
#include "kutils.h"
#include "mapped_buffer.h"

/* Return the buffer associated with pid */
struct mapped_buffer *get_buffer(pid_t pid, struct mapped_buffer *buffers)
{
        struct mapped_buffer *buffer;

        list_for_each_entry(buffer, &buffers->list, list)
                if (buffer->pid == pid)
                        return buffer;
        return NULL;
}

void swap_buffer_cache(struct mapped_buffer *buffer)
{
        void *buf;
        struct vm_area_struct *vma;
        unsigned long start;

        buf = buffer->buffer;
        vma = buffer->vma;
        start = buffer->start;

        buffer->buffer = buffer->cache;
        buffer->vma = buffer->cache_vma;
        buffer->start = buffer->cache_start;

        buffer->cache = buf;
        buffer->cache_vma = vma;
        buffer->cache_start = start;
}

/* Free buffer associated with pid */
void remove_buffer(pid_t pid, struct mapped_buffer *buffers)
{
        struct mapped_buffer *buffer;

        buffer = get_buffer(pid, buffers);
        if (buffer) {
                if (buffer->buffer)
                        vfree(buffer->buffer);
                if (buffer->cache)
                        vfree(buffer->cache);
                list_del(&buffer->list);
                kfree(buffer);
        }
}

/* Add a new buffer for pid */
struct mapped_buffer *add_buffer(pid_t pid, struct mapped_buffer *buffers)
{
        struct mapped_buffer *buffer;

        buffer = kmalloc(sizeof(*buffer), GFP_KERNEL);
        if (buffer == NULL)
                return NULL;
        buffer->pid = pid;
        buffer->buffer = vmalloc_user(PAGE_SIZE);
        if (buffer->buffer == NULL) {
                kfree(buffer);
                return NULL;
        }
        buffer->cache = vmalloc_user(PAGE_SIZE);
        if (buffer->cache == NULL) {
                kfree(buffer->buffer);
                kfree(buffer);
                return NULL;
        }
        buffer->vma = NULL;
        buffer->cache_vma = NULL;
        INIT_LIST_HEAD(&buffer->list);
        list_add_tail(&buffer->list, &buffers->list);
        return buffer;
}

/* Print buffers */
void dump_buffers(struct mapped_buffer *buffers)
{
        struct mapped_buffer *buffer;

        PR_DEBUG(D_BUF, "Buffers:");
        list_for_each_entry(buffer, &buffers->list, list)
                PR_DEBUG(D_BUF, "- Buffer: pid = %d, address = %016lx", buffer->pid, buffer->start);
        PR_DEBUG(D_BUF, "");
}

/* Init buffer list */
void init_buffers(struct mapped_buffer *buffers)
{
        INIT_LIST_HEAD(&buffers->list);
}

/* Free all buffers */
void remove_buffers(struct mapped_buffer *buffers)
{
        struct mapped_buffer *buffer, *tmp;

        list_for_each_entry_safe(buffer, tmp, &buffers->list, list) {
                if (buffer->buffer)
                        vfree(buffer->buffer);
                if (buffer->cache)
                        vfree(buffer->cache);
                list_del(&buffer->list);
                kfree(buffer);
        }
}
