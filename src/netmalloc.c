/*
** netmalloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Fri May  9 11:34:27 2014 xsyann
** Last update Wed May 21 17:28:17 2014 xsyann
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

static struct mapped_buffer buffers;
static struct area_struct area_list;

static struct storage_ops storage_ops = {
        .save = kernel_save,
        .load = kernel_load,
        .release = kernel_release
};

static struct mutex mutex;

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
static int copy_page_from_user(pid_t pid, unsigned long start,
                               struct vm_area_struct *vma)
{
        struct page *page;
        char *buffer;
        int res;

        down_read(&vma->vm_mm->mmap_sem);
        res = get_user_pages(NULL, vma->vm_mm, start, 1, 0, 0, &page, NULL);
        PR_DEBUG("Copy from user success = %d, pid = %d", res, pid);
        if (res == 1) {
                buffer = kmap(page);
                if (buffer == NULL) {
                        up_read(&vma->vm_mm->mmap_sem);
                        return -ENOMEM;
                }
                /* Copy data to storage */
                storage_ops.save(pid, start, buffer);

                kunmap(page);
                put_page(page);
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

/* Return the buffer associated with pid */
static struct mapped_buffer *get_buffer(pid_t pid)
{
        struct mapped_buffer *buffer;

        list_for_each_entry(buffer, &buffers.list, list) {
                if (buffer->pid == pid)
                        return buffer;
        }
        return NULL;
}

/* Copy userspace page to kernel and save data (copy_page_from_user).
 * Remove page from user address space (remove_user_page). */
static int unmap_page(pid_t pid)
{
        struct mapped_buffer *buffer = get_buffer(pid);

        if (buffer && buffer->vma) {
                if (copy_page_from_user(buffer->pid, buffer->start, buffer->vma))
                        return -1;
                PR_DEBUG("Unmap page %016lx (page %ld) pid = %d", buffer->start,
                         (buffer->start - buffer->vma->vm_start) >> PAGE_SHIFT, pid);
                if (remove_user_page(buffer->start, buffer->vma))
                        return -1;
                buffer->vma = NULL;
        }
        return 0;
}

/* Add a new buffer for pid */
static struct mapped_buffer *add_buffer(pid_t pid)
{
        struct mapped_buffer *buffer;

        PR_DEBUG("ADD BUFFER");
        buffer = kmalloc(sizeof(*buffer), GFP_KERNEL);
        if (buffer == NULL)
                return NULL;
        buffer->pid = pid;
        buffer->buffer = vmalloc_user(PAGE_SIZE);
        if (buffer->buffer == NULL) {
                kfree(buffer);
                return NULL;
        }
        buffer->vma = NULL;
        INIT_LIST_HEAD(&buffer->list);
        list_add_tail(&buffer->list, &buffers.list);
        return buffer;
}

/* Free all buffers */
static void remove_buffers(void)
{
        struct mapped_buffer *buffer, *tmp;

        list_for_each_entry_safe(buffer, tmp, &buffers.list, list) {
                if (buffer->buffer)
                        vfree(buffer->buffer);
                list_del(&buffer->list);
                kfree(buffer);
        }
}

/* Fill buffer and insert it into user address space. */
static struct page *map_page(pid_t pid, unsigned long address,
                             struct vm_area_struct *vma)
{
        struct page *page;
        struct mapped_buffer *buffer = get_buffer(pid);

        if (buffer == NULL)
                buffer = add_buffer(pid);
        if (buffer == NULL)
                return NULL;

        page = vmalloc_to_page(buffer->buffer);
        get_page(page);

        /* Load data from storage */
        storage_ops.load(pid, address, buffer->buffer);

        PR_DEBUG("Map page %016lx (page %ld) pid = %d",
                 address, (address - vma->vm_start) >> PAGE_SHIFT, pid);
        if (vm_insert_page(vma, address, page))
                return NULL;
        buffer->vma = vma;
        buffer->start = address;
        return page;
}

/* Return the page corresponding to address, filled with stored data. */
static struct page *netmalloc_get_page(pid_t pid,
                                       unsigned long address,
                                       struct vm_area_struct *vma)
{
        struct page *page;

        /* Unmap and save old */
        if (unmap_page(pid))
                return NULL;

        /* Fill and map new */
        page = map_page(pid, address, vma);
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
        struct mapped_buffer *buffer;

        mutex_lock_interruptible(&mutex);
        PR_DEBUG("Close mutex acquired");
        remove_vma_from_area_list(&area_list, vma);

        /* To avoid unmapping in closed vma */
        list_for_each_entry(buffer, &buffers.list, list) {
                if (buffer->vma == vma)
                        buffer->vma = NULL;
        }

        mutex_unlock(&mutex);
}

/* Fault handler. Called when user access unallocated page. */
static int netmalloc_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
        struct page *page;
        unsigned long address = (unsigned long)(vmf->virtual_address);

        PR_DEBUG("Fault at %p (page %ld)", vmf->virtual_address,
                 (address - vma->vm_start) >> PAGE_SHIFT);

        mutex_lock_interruptible(&mutex);
        PR_DEBUG("Fault mutex acquired");

        /* Return SIGBUS if user isn't allowed to access this vma page. */
        if (!is_valid_address(address, vma)) {
                mutex_unlock(&mutex);
                return VM_FAULT_SIGBUS;
        }

        page = netmalloc_get_page(current->pid, address, vma);
        if (page == NULL) {
                mutex_unlock(&mutex);
                PR_WARN(NETMALLOC_WARN_PAGE);
                return VM_FAULT_RETRY;
        }

        vmf->page = page;

        mutex_unlock(&mutex);

        PR_DEBUG("----- Fault Handled\n");

        return 0;
}

static struct vm_operations_struct netmalloc_vm_ops = {
        .open = netmalloc_vm_open,
        .close = netmalloc_vm_close,
        .fault = netmalloc_vm_fault
};

/* ************************************************************ */

static void *netmalloc_syscall(unsigned long size)
{
        static struct region_struct *region;
        void *start;

        PR_DEBUG("Netmalloc %ld bytes in process %d (%s)",
                size, current->pid, current->comm);

        mutex_lock_interruptible(&mutex);
        PR_DEBUG("Syscall mutex acquired");

        region = create_region(size, current, &area_list, &netmalloc_vm_ops);
        if (region == NULL) {
                mutex_unlock(&mutex);
                return NULL;
        }
        /* Unmap old page to be sure to reload data if region is created in the
           same vma page as the mapped current one. */
        unmap_page(current->pid);
        dump_area_list();

        start = region->virtual_start;

        mutex_unlock(&mutex);

        return start;
}

/* ************************************************************ */

static int __init netmalloc_init_module(void)
{
        PR_INFO(NETMALLOC_INFO_LOAD);
        replace_syscall(SYSCALL_NI, (ulong)netmalloc_syscall);

        init_area_list(&area_list);
        INIT_LIST_HEAD(&buffers.list);

        mutex_init(&mutex);

        return 0;
}

static void __exit netmalloc_exit_module(void)
{
        PR_INFO(NETMALLOC_INFO_UNLOAD);
        restore_syscall(SYSCALL_NI);
        remove_buffers();
        remove_area_list(&area_list);
        storage_ops.release();
}

module_init(netmalloc_init_module);
module_exit(netmalloc_exit_module);
