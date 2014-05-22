/*
** vma.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May 16 18:07:20 2014 xsyann
** Last update Thu May 22 14:48:50 2014 xsyann
*/

#ifndef         __VMA_H__
#define         __VMA_H__

#include <linux/mm.h>

typedef int insert_vm_prot(struct mm_struct *, struct vm_area_struct *);
typedef int do_munmap_prot(struct mm_struct *, unsigned long, size_t);

struct vm_area_struct *add_vma(struct task_struct *task,
                               struct vm_operations_struct *vm_ops,
                               unsigned long size);

void remove_vma(struct vm_area_struct *vma);
int is_last_vma(struct vm_area_struct *vma);
int is_existing_vma(struct vm_area_struct *vma);

#endif          /* __VMA_H__ */
