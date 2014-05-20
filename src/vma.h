/*
** vma.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May 16 18:07:20 2014 xsyann
** Last update Tue May 20 04:29:13 2014 xsyann
*/

#ifndef         __VMA_H__
#define         __VMA_H__

#include <linux/mm.h>

typedef int insert_vm_prot(struct mm_struct *, struct vm_area_struct *);

struct vm_area_struct *add_vma(struct task_struct *task,
                               struct vm_operations_struct *vm_ops,
                               unsigned long size);

int is_last_vma(struct vm_area_struct *vma);

#endif          /* __VMA_H__ */
