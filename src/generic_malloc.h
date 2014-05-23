/*
** generic_malloc.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Wed May 21 20:19:46 2014 xsyann
** Last update Fri May 23 07:47:39 2014 xsyann
*/

#ifndef         __GENERIC_MALLOC_H__
#define         __GENERIC_MALLOC_H__

#include <linux/mm.h>
#include "storage.h"

struct stored_page
{
        pid_t pid;
        unsigned long start;
        struct list_head list;
};

struct mapped_buffer
{
        void *buffer;
        struct vm_area_struct *vma;
        unsigned long start;
        pid_t pid;
        struct list_head list;
};

typedef void zap_page_range_prot(struct vm_area_struct *, unsigned long,
                                 unsigned long, struct zap_details *);

#define VMA_SIZE PAGE_SIZE * 2

void generic_free(void *ptr);
void *generic_malloc(unsigned long size);
void generic_malloc_init(struct storage_ops *s_ops);
void generic_malloc_clean(void);

#endif          /* __GENERIC_MALLOC_H__ */
