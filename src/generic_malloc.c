/*
** generic_malloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Wed May 21 20:08:11 2014 xsyann
** Last update Sat May 24 18:55:29 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/pagemap.h>

#include "syscall.h"
#include "vma.h"
#include "area.h"
#include "storage.h"
#include "stored_page.h"
#include "mapped_buffer.h"
#include "kutils.h"
#include "netmalloc.h"
#include "generic_malloc.h"

/* ************************************************************ */

static struct mapped_buffer buffers;
static struct stored_page stored_pages;
static struct area_struct area_list;

static struct storage_ops *storage_ops;

static struct semaphore sem;

/* ************************************************************ */

/* Remove all pages in storage for region at address */
void remove_storage_pages(pid_t pid, unsigned long address)
{
        struct area_struct *area;
        struct region_struct *region;
        unsigned long start, end;

        area = get_area(pid, address, &area_list);
        if (area) {
                region = get_region(area, address);
                if (region) {
                        start = (unsigned long)region->virtual_start;
                        end = (unsigned long)region->virtual_start + region->size;
                        /* Page align */
                        start = align_floor(start, PAGE_SHIFT);
                        end = align_ceil(end, PAGE_SHIFT);
                        for (; start < end; start += PAGE_SIZE)
                                if (is_stored_page(pid, start, &stored_pages)) {
                                        PR_DEBUG(D_MED, "Remove data at %016lx, pid = %d",
                                                 start, pid);
                                        storage_ops->remove(pid, start);
                                        remove_stored_page(pid, start, &stored_pages);
                                }
                }
        }
}

/* Free area regions from storage and remove from list
 * Free pid buffer if no more vma for pid. */
static void close_vma(struct vm_area_struct *vma)
{
        struct area_struct *area;
        struct region_struct *region, *tmp_region;

        list_for_each_entry(area, &area_list.list, list) {
                if (area->vma == vma) {
                        list_for_each_entry_safe(region, tmp_region,
                                                 &area->regions.list, list) {
                                remove_storage_pages(area->pid,
                                                     (unsigned long)region->virtual_start);
                                list_del(&region->list);
                                kfree(region);
                        }
                        list_del(&area->list);
                        if (get_area_count(area->pid, &area_list) == 0)
                                remove_buffer(area->pid, &buffers);
                        kfree(area);
                        break;
                }
        }
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
                if (add_stored_page(pid, start, &stored_pages) == NULL)
                        return -ENOMEM;
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

/* Copy userspace page to kernel and save data (copy_page_from_user).
 * Remove page from user address space (remove_user_page). */
static int unmap_page(pid_t pid)
{
        struct mapped_buffer *buffer = get_buffer(pid, &buffers);

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

/* Fill buffer and insert it into user address space. */
static struct page *map_page(pid_t pid, unsigned long address,
                             struct vm_area_struct *vma)
{
        struct page *page;
        struct mapped_buffer *buffer = get_buffer(pid, &buffers);

        if (buffer == NULL)
                buffer = add_buffer(pid, &buffers);
        if (buffer == NULL)
                return NULL;

        page = vmalloc_to_page(buffer->buffer);
        get_page(page);

        /* Load data from storage */
        if (is_stored_page(pid, address, &stored_pages)) {
                storage_ops->load(pid, address, buffer->buffer);
                PR_DEBUG(D_MED, "Load data at %016lx (page %ld) pid = %d",
                         address, (address - vma->vm_start) >> PAGE_SHIFT, pid);
        }
        else
                memset(buffer->buffer, 0, PAGE_SIZE);
        if (vm_insert_page(vma, address, page))
                return NULL;
        buffer->vma = vma;
        buffer->start = address;
        return page;
}

/* Return the page corresponding to address, filled with stored data. */
static struct page *generic_malloc_get_page(pid_t pid,
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

        /* To avoid unmapping in closed vma */
        list_for_each_entry(buffer, &buffers.list, list) {
                if (buffer->vma == vma)
                        buffer->vma = NULL;
        }

        close_vma(vma);

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
        if (!is_valid_address(address, vma, &area_list)) {
                up(&sem);
                return VM_FAULT_SIGBUS;
        }

        page = generic_malloc_get_page(current->pid, address, vma);
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
        pid_t pid;

        PR_DEBUG(D_MIN, "Free call at %p, pid = %d", ptr, current->pid);

        down_write(&current->mm->mmap_sem);
        down_interruptible(&sem);
        PR_DEBUG(D_MIN, "Free exec %p, pid = %d", ptr, current->pid);

        pid = current->pid;
        unmap_page(pid);
        remove_storage_pages(pid, (unsigned long)ptr);
        remove_region(pid, (unsigned long)ptr, &area_list);

        dump_area_list(&area_list); /* Debug */

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

        dump_area_list(&area_list); /* Debug */

        start = region->virtual_start;

        up(&sem);
        up_write(&current->mm->mmap_sem);
        PR_DEBUG(D_MIN, "Alloc end for %ld bytes in process %d (%s)\n",
                 size, current->pid, current->comm);

        return start;
}

int generic_malloc_init(struct storage_ops *s_ops, void *param)
{
        int error;

        storage_ops = s_ops;

        if ((error = storage_ops->init(param))) {
                PR_WARN(NETMALLOC_WARN_STORAGE);
                return error;
        }

        init_area_list(&area_list);
        init_stored_pages(&stored_pages);
        init_buffers(&buffers);

        sema_init(&sem, 1);
        return 0;
}

void generic_malloc_clean(void)
{
        dump_area_list(&area_list);
        remove_buffers(&buffers);
        remove_stored_pages(&stored_pages);
        remove_area_list(&area_list);
        storage_ops->release();
}

