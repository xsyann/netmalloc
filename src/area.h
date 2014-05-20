/*
** area.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Tue May 20 04:26:59 2014 xsyann
** Last update Tue May 20 04:58:19 2014 xsyann
*/

#ifndef         __AREA_H__
#define         __AREA_H__

#include <linux/mm.h>

struct region_struct
{
        unsigned long size;
        struct list_head list;
        void *virtual_start;
};

struct area_struct
{
        struct vm_area_struct *vma;
        pid_t pid;
        unsigned long size;
        unsigned long free_space;
        struct region_struct regions;
        struct list_head list;
};


void init_area_list(struct area_struct *area_list);
void remove_area_list(struct area_struct *area_list);
struct region_struct *create_region(unsigned long size,
                                    struct task_struct *task,
                                    struct area_struct *area_list,
                                    struct vm_operations_struct *vm_ops);

#endif          /* __AREA_H__ */
