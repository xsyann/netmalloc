/*
** netmalloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:27 2014 xsyann
** Last update Wed May 21 11:28:47 2014 xsyann
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
#include "storage.h"
#include "netmalloc.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR(NETMALLOC_AUTHOR);
MODULE_DESCRIPTION(NETMALLOC_DESC);
MODULE_VERSION(NETMALLOC_VERSION);

/* ************************************************************ */

static struct mapped_buffer mapped_buffer;
static struct area_struct area_list;

static struct storage_ops storage_ops = {
        .save = kernel_save,
        .load = kernel_load,
        .release = kernel_release
};

/* ************************************************************ */

/* Print all areas and regions */
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

/* Copy userspace page to kernel and save data. */
static int copy_page_from_user(unsigned long start, struct vm_area_struct *vma)
{
        struct page *page;
        char *buffer;
        int res;

        down_read(&vma->vm_mm->mmap_sem);
        res = get_user_pages(NULL, vma->vm_mm, start, 1, 0, 0, &page, NULL);
        PR_DEBUG("Copy from user success = %d", res);
        if (res == 1) {
                buffer = kmap(page);
                if (buffer == NULL) {
                        up_read(&vma->vm_mm->mmap_sem);
                        return -ENOMEM;
                }
                /* Copy data to storage */
                storage_ops.save(start, buffer);

                kunmap(page);
                up_read(&vma->vm_mm->mmap_sem);
        }
        return 0;
}

/* Remove mapped page from user address space.
 * Return 0 on success and negative on error. */
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

/* Copy userspace page to kernel to save data.
 * Remove page from user address space. */
static int unmap_page(void)
{
        if (mapped_buffer.vma) {
                if (copy_page_from_user(mapped_buffer.start, mapped_buffer.vma))
                        return -1;
                PR_DEBUG("Unmap page %016lx (page %ld)", mapped_buffer.start,
                         (mapped_buffer.start - mapped_buffer.vma->vm_start) >> PAGE_SHIFT);
                if (remove_user_page(mapped_buffer.start, mapped_buffer.vma))
                        return -1;
                mapped_buffer.vma = NULL;
        }
        return 0;
}

/* Fill buffer and insert it into user address space. */
static struct page *map_page(unsigned long address, struct vm_area_struct *vma)
{
        struct page *page;

        page = vmalloc_to_page(mapped_buffer.buffer);
        get_page(page);

        /* Load data from storage */
        storage_ops.load(address, mapped_buffer.buffer);

        PR_DEBUG("Map page %016lx (page %ld)",
                 address, (address - vma->vm_start) >> PAGE_SHIFT);
        if (vm_insert_page(vma, address, page))
                return NULL;
        mapped_buffer.vma = vma;
        mapped_buffer.start = address;
        return page;
}

/* Return the page corresponding to address, filled with stored data. */
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

/* ************************************************************ */

static void netmalloc_vm_open(struct vm_area_struct *vma)
{
        PR_DEBUG("open");
}

static void netmalloc_vm_close(struct vm_area_struct *vma)
{
        PR_DEBUG("close");
}

/* Fault handler. Called when user access unallocated page. */
static int netmalloc_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
        struct page *page;
        unsigned long address = (unsigned long)(vmf->virtual_address);

        PR_DEBUG("Fault at %p (page %ld)", vmf->virtual_address,
                 (address - vma->vm_start) >> PAGE_SHIFT);

        /* Return SIGBUS if user isn't allowed to access this vma page. */
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

/* ************************************************************ */

/* NetMalloc syscall */
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
        PR_INFO(NETMALLOC_INFO_UNLOAD);
        restore_syscall(SYSCALL_NI);
        if (mapped_buffer.buffer != NULL)
                vfree(mapped_buffer.buffer);
        remove_area_list(&area_list);
        storage_ops.release();
}

module_init(netmalloc_init_module);
module_exit(netmalloc_exit_module);
