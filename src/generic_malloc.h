/*
** generic_malloc.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Wed May 21 20:19:46 2014 xsyann
** Last update Sat May 24 01:46:51 2014 xsyann
*/

#ifndef         __GENERIC_MALLOC_H__
#define         __GENERIC_MALLOC_H__

#include <linux/mm.h>
#include "storage.h"

typedef void zap_page_range_prot(struct vm_area_struct *, unsigned long,
                                 unsigned long, struct zap_details *);

#define VMA_SHIFT (PAGE_SHIFT + 1)

void generic_free(void *ptr);
void *generic_malloc(unsigned long size);
void generic_malloc_init(struct storage_ops *s_ops);
void generic_malloc_clean(void);

#endif          /* __GENERIC_MALLOC_H__ */
