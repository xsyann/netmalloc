/*
** netmalloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:27 2014 xsyann
** Last update Tue May 20 21:47:58 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/pagemap.h>

#include "syscall.h"
#include "vma.h"
#include "area.h"
#include "netmalloc.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(NETMALLOC_AUTHOR);
MODULE_DESCRIPTION(NETMALLOC_DESC);
MODULE_VERSION(NETMALLOC_VERSION);

struct mapped_buffer
{
        void *buffer;
        struct vm_area_struct *vma;
        unsigned long start;
};

typedef void zap_page_range_prot(struct vm_area_struct *, unsigned long,
                                 unsigned long, struct zap_details *);

static struct mapped_buffer mapped_buffer;
struct area_struct area_list;

static void netmalloc_vm_open(struct vm_area_struct *vma)
{
        PR_DEBUG("open");
}

static void netmalloc_vm_close(struct vm_area_struct *vma)
{
        PR_DEBUG("close");
}

/* Return area_struct corresponding to vma */
/*
static struct area_struct *get_area_struct(struct vm_area_struct *vma)
{
        struct area_struct *area;
        list_for_each_entry(area, &area_list.list, list) {
                if (area->vma == vma)
                        return area;
        }
        return NULL;
}
*/

static void dump_area_list(void)
{
        struct area_struct *area;
        struct region_struct *region;
        PR_DEBUG("\n");
        PR_DEBUG("Area list :");
        list_for_each_entry(area, &area_list.list, list) {
                PR_DEBUG("- VMA : size = %ld, pid = %d, freespace = %ld",
                        area->size, area->pid, area->free_space);
                list_for_each_entry(region, &area->regions.list, list) {
                        PR_DEBUG("   - Region, size = %ld, start = %p",
                                 region->size, region->virtual_start);
                }
        }
        PR_DEBUG("\n");
}

/* Return 1 if address is a valid virtual address and 0 if not. */
static int is_valid_address(unsigned long address, struct vm_area_struct *vma)
{
        struct area_struct *area;
        struct region_struct *region;
        unsigned long start, end;

        list_for_each_entry(area, &area_list.list, list) {
                if (area->vma == vma)
                        list_for_each_entry(region, &area->regions.list, list) {
                                start = (unsigned long)region->virtual_start;
                                end = start + region->size;
                                if (address >= start && address < end)
                                        return 1;
                        }
        }
        return 0;
}

static void copy_page_from_user(unsigned long start, struct vm_area_struct *vma)
{
        struct page *page;
        char *buffer;
        int res;

        down_read(&vma->vm_mm->mmap_sem);
        res = get_user_pages(NULL, vma->vm_mm, start, 1, 0, 0, &page, NULL);
        PR_DEBUG("Copy from user success = %d", res);
        if (res == 1)
        {
                buffer = kmap(page);
                /* Copy data to storage */
                /* here */
                PR_DEBUG("page[2] = %d", buffer[2]);
                kunmap(page);
                up_read(&vma->vm_mm->mmap_sem);
        }
}

/* Return 0 on success and negative on error. */
static int remove_user_page(unsigned long start, struct vm_area_struct *vma)
{
        ulong* zap_page_range;
        down_read(&vma->vm_mm->mmap_sem);
        zap_page_range = (ulong*)kallsyms_lookup_name("zap_page_range");
        if (zap_page_range == NULL)
                return -1;
        ((zap_page_range_prot *)zap_page_range)(vma, start, PAGE_SIZE, NULL);
        up_read(&vma->vm_mm->mmap_sem);
        return 0;
}

static int unmap_page(void)
{
        if (mapped_buffer.vma) {
                copy_page_from_user(mapped_buffer.start, mapped_buffer.vma);
                PR_DEBUG("Unmap page %016lx (page %ld)", mapped_buffer.start,
                         (mapped_buffer.start - mapped_buffer.vma->vm_start) >> PAGE_SHIFT);
                if (remove_user_page(mapped_buffer.start, mapped_buffer.vma))
                        return -1;
                mapped_buffer.vma = NULL;
        }
        return 0;
}

static struct page *map_page(unsigned long address, struct vm_area_struct *vma)
{
        struct page *page;

        page = vmalloc_to_page(mapped_buffer.buffer);
        get_page(page);

        /* Load data from storage */
        /* here */

        PR_DEBUG("Map page %016lx (page %ld)",
                 address, (address - vma->vm_start) >> PAGE_SHIFT);
        if (vm_insert_page(vma, address, page))
                return NULL;
        mapped_buffer.vma = vma;
        mapped_buffer.start = address;
        return page;
}

static struct page *netmalloc_get_page(unsigned long address,
                                       struct vm_area_struct *vma)
{
        struct page *page;

        /* Unmap and save old */
        if (unmap_page())
                return NULL;

        /* Fill and map new */
        page = map_page(address, vma);
        if (page == NULL)
                return NULL;

        return page;
}

static int netmalloc_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
        struct page *page;
        unsigned long address = (unsigned long)(vmf->virtual_address);

        PR_DEBUG("Fault at %p (page %ld)", vmf->virtual_address,
                 (address - vma->vm_start) >> PAGE_SHIFT);

        if (!is_valid_address(address, vma))
                return VM_FAULT_SIGBUS;

        page = netmalloc_get_page(address, vma);
        if (page == NULL) {
                PR_WARN(NETMALLOC_WARN_PAGE);
                return VM_FAULT_RETRY;
        }
        vmf->page = page;
        PR_DEBUG("----- Fault Handled\n");
        return 0;
}

static struct vm_operations_struct netmalloc_vm_ops = {
        .open = netmalloc_vm_open,
        .close = netmalloc_vm_close,
        .fault = netmalloc_vm_fault
};

static void *netmalloc_syscall(unsigned long size)
{
        static struct region_struct *region;

        PR_DEBUG("Netmalloc %ld bytes in process %d (%s)",
                size, current->pid, current->comm);

        region = create_region(size, current, &area_list, &netmalloc_vm_ops);
        if (region == NULL)
                return NULL;
        /* Unmap old page to be sure to reload data if region is created in the
           same vma page as the mapped current one. */
        unmap_page();
        dump_area_list();
        return region->virtual_start;
}

/* ************************************************************ */

static int __init netmalloc_init_module(void)
{
        PR_INFO(NETMALLOC_INFO_LOAD);
        replace_syscall(SYSCALL_NI, (ulong)netmalloc_syscall);
        init_area_list(&area_list);

        mapped_buffer.buffer = vmalloc_user(PAGE_SIZE);
        if (mapped_buffer.buffer == NULL)
                return -ENOMEM;
        mapped_buffer.vma = NULL;

        return 0;
}

static void __exit netmalloc_exit_module(void)
{
        restore_syscall(SYSCALL_NI);
        if (mapped_buffer.buffer != NULL)
                vfree(mapped_buffer.buffer);
        remove_area_list(&area_list);
        PR_INFO(NETMALLOC_INFO_UNLOAD);
}

module_init(netmalloc_init_module);
module_exit(netmalloc_exit_module);
