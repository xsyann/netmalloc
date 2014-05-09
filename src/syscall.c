/*
** syscall.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 12:13:19 2014 xsyann
** Last update Fri May  9 12:18:28 2014 xsyann
*/

#include <linux/kallsyms.h>
#include "syscall.h"

#define SYS_CALL_TABLE "sys_call_table"

static ulong *syscall_table = (ulong *)0xffffffff81801420;
static void *original_syscall = NULL;

static int is_syscall_table(ulong *p)
{
        return ((p != NULL) && (p[__NR_close] == (ulong)sys_close));
}

static int page_read_write(ulong address)
{
        uint level;
        pte_t *pte = lookup_address(address, &level);

        if(pte->pte &~ _PAGE_RW)
                pte->pte |= _PAGE_RW;
        return 0;
}

static int page_read_only(ulong address)
{
        uint level;
        pte_t *pte = lookup_address(address, &level);
        pte->pte = pte->pte &~ _PAGE_RW;
        return 0;
}

void replace_syscall(ulong offset, ulong new_syscall)
{
        syscall_table = (ulong *)kallsyms_lookup_name(SYS_CALL_TABLE);
        if (is_syscall_table(syscall_table)) {
                page_read_write((ulong)syscall_table);
                original_syscall = (void *)(syscall_table[offset]);
                syscall_table[offset] = new_syscall;
                page_read_only((ulong)syscall_table);
        }
}

void restore_syscall(ulong offset)
{
        page_read_write((ulong)syscall_table);
        syscall_table[offset] = (ulong)original_syscall;
        page_read_only((ulong)syscall_table);
}
