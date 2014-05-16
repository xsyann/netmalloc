/*
** memory.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May 16 18:07:20 2014 xsyann
** Last update Fri May 16 18:17:43 2014 xsyann
*/

#ifndef         __MEMORY_H__
#define         __MEMORY_H__

#include <linux/mm.h>

typedef int insert_vm_prot(struct mm_struct *, struct vm_area_struct *);

struct vm_area_struct *add_vma(struct task_struct *task,
                               struct vm_operations_struct *vm_ops);

#endif          /* __MEMORY_H__ */
