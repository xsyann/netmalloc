/*
** generic_malloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Wed May 21 20:08:11 2014 xsyann
** Last update Thu May 22 21:44:14 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/pagemap.h>

#include "syscall.h"
#include "vma.h"
#include "area.h"
#include "storage.h"
#include "kutils.h"
#include "netmalloc.h"
#include "generic_malloc.h"

/* ************************************************************ */

static struct mapped_buffer buffers;
static struct area_struct area_list;

static struct storage_ops *storage_ops;

static struct semaphore sem;

/* ************************************************************ */

/* Print all areas and regions */
static void dump_area_list(void)
{
        struct area_struct *area;
        struct region_struct *region;
        PR_DEBUG(D_MED, "");
        PR_DEBUG(D_MED, "Area list :");
        list_for_each_entry(area, &area_list.list, list) {
                PR_DEBUG(D_MED, "- VMA : size = %ld, pid = %d, freespace = %ld",
                        area->size, area->pid, area->free_space);
                list_for_each_entry(region, &area->regions.list, list) {
                        PR_DEBUG(D_MED, "   - Region, size = %ld, start = %p, free = %d",
                                 region->size, region->virtual_start, region->free);
                }
        }
              PR_DEBUG(D_MED, "");
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
                                if (address >= start && address < end &&
                                    region->free == 0)
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

        res = get_user_pages(NULL, vma->vm_mm, start, 1, 0, 0, &page, NULL);
        PR_DEBUG(D_MED, "Get user page success = %d, pid = %d", res, pid);
        if (res == 1) {
                buffer = kmap(page);
                if (buffer == NULL)
                        return -ENOMEM;
                storage_ops->save(pid, start, buffer);

                kunmap(page);
                put_page(page);
        }
        return 0;
}

/* Remove mapped page from user address space.
 * Return 0 on success and negative on error. */
static int remove_user_page(unsigned long start, struct vm_area_struct *vma)
{
        ulong* zap_page_range;

        zap_page_range = (ulong*)kallsyms_lookup_name("zap_page_range");
        if (zap_page_range == NULL)
                return -1;
        ((zap_page_range_prot *)zap_page_range)(vma, start, PAGE_SIZE, NULL);
        return 0;
}

/* Return the buffer associated with pid */
static struct mapped_buffer *get_buffer(pid_t pid)
{
        struct mapped_buffer *buffer;

        list_for_each_entry(buffer, &buffers.list, list)
                if (buffer->pid == pid)
                        return buffer;
        return NULL;
}

/* Copy userspace page to kernel and save data (copy_page_from_user).
 * Remove page from user address space (remove_user_page). */
static int unmap_page(pid_t pid)
{
        struct mapped_buffer *buffer = get_buffer(pid);

        if (buffer && buffer->vma && is_existing_vma(buffer->vma)) {
                if (copy_page_from_user(buffer->pid, buffer->start, buffer->vma))
                        return -1;
                PR_DEBUG(D_MED, "Store data at %016lx (page %ld) pid = %d", buffer->start,
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
        storage_ops->load(pid, address, buffer->buffer);

        PR_DEBUG(D_MED, "Load data at %016lx (page %ld) pid = %d",
                 address, (address - vma->vm_start) >> PAGE_SHIFT, pid);
        if (vm_insert_page(vma, address, page))
                return NULL;
        buffer->vma = vma;
        buffer->start = address;
        return page;
}

/* Return the page corresponding to address, filled with stored data. */
static struct page *generic_alloc_get_page(pid_t pid,
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

static void generic_malloc_vm_open(struct vm_area_struct *vma)
{
        PR_DEBUG(D_MIN, "open");
}

static void generic_malloc_vm_close(struct vm_area_struct *vma)
{
        struct mapped_buffer *buffer;

        PR_DEBUG(D_MIN, "Close call pid = %d, vma = %p", current->pid, vma);
        PR_DEBUG(D_MIN, "Close exec pid = %d, vma = %p", current->pid, vma);
        remove_vma_from_area_list(&area_list, vma);

        /* To avoid unmapping in closed vma */
        list_for_each_entry(buffer, &buffers.list, list) {
                if (buffer->vma == vma)
                        buffer->vma = NULL;
        }
        PR_DEBUG(D_MIN, "Close end pid = %d, vma = %p\n", current->pid, vma);
}

/* Fault handler. Called when user access unallocated page. */
static int generic_malloc_vm_fault(struct vm_area_struct *vma,
                                  struct vm_fault *vmf)
{
        struct page *page;
        unsigned long address = (unsigned long)(vmf->virtual_address);

        PR_DEBUG(D_MIN, "Fault call at %p (page %ld), pid = %d", vmf->virtual_address,
                 (address - vma->vm_start) >> PAGE_SHIFT, current->pid);

        down_interruptible(&sem);
        PR_DEBUG(D_MIN, "Fault exec at %p (page %ld), pid = %d", vmf->virtual_address,
                 (address - vma->vm_start) >> PAGE_SHIFT, current->pid);

        /* Return SIGBUS if user isn't allowed to access this vma page. */
        if (!is_valid_address(address, vma)) {
                up(&sem);
                return VM_FAULT_SIGBUS;
        }

        page = generic_alloc_get_page(current->pid, address, vma);
        if (page == NULL) {
                up(&sem);
                PR_WARN(NETMALLOC_WARN_PAGE);
                return VM_FAULT_RETRY;
        }

        vmf->page = page;

        up(&sem);

        PR_DEBUG(D_MIN, "Fault end at %p (page %ld), pid = %d\n", vmf->virtual_address,
                 (address - vma->vm_start) >> PAGE_SHIFT, current->pid);

        return 0;
}

static struct vm_operations_struct vm_ops = {
        .open = generic_malloc_vm_open,
        .close = generic_malloc_vm_close,
        .fault = generic_malloc_vm_fault
};

/* ************************************************************ */

void generic_free(void *ptr)
{
        PR_DEBUG(D_MIN, "Free call at %p, pid = %d", ptr, current->pid);

        down_write(&current->mm->mmap_sem);
        down_interruptible(&sem);
        PR_DEBUG(D_MIN, "Free exec %p, pid = %d", ptr, current->pid);

        unmap_page(current->pid);
        remove_region(current->pid, (unsigned long)ptr, &area_list);
        storage_ops->remove(current->pid, (unsigned long)ptr);

        dump_area_list(); /* Debug */

        up(&sem);
        up_write(&current->mm->mmap_sem);
        PR_DEBUG(D_MIN, "Free end %p, pid %d\n", ptr, current->pid);
}

void *generic_malloc(unsigned long size)
{
        struct region_struct *region;
        void *start;

        PR_DEBUG(D_MIN, "Alloc call for %ld bytes in process %d (%s)",
                 size, current->pid, current->comm);

        down_write(&current->mm->mmap_sem);
        down_interruptible(&sem);
        PR_DEBUG(D_MIN, "Alloc exec for %ld bytes in process %d (%s)",
                 size, current->pid, current->comm);

        region = create_region(size, current, &area_list, &vm_ops);
        if (region == NULL) {
                up(&sem);
                return NULL;
        }

        /* Unmap old page to be sure to reload data if region is created in the
           same vma page as the mapped current one. */
        unmap_page(current->pid);

        dump_area_list(); /* Debug */

        start = region->virtual_start;

        up(&sem);
        up_write(&current->mm->mmap_sem);
        PR_DEBUG(D_MIN, "Alloc end for %ld bytes in process %d (%s)\n",
                 size, current->pid, current->comm);

        return start;
}

void generic_malloc_init(struct storage_ops *s_ops)
{
        storage_ops = s_ops;
        init_area_list(&area_list);
        INIT_LIST_HEAD(&buffers.list);
        sema_init(&sem, 1);
}

void generic_malloc_clean(void)
{
        remove_buffers();
        remove_area_list(&area_list);
        storage_ops->release();
}

