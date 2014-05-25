/*
** generic_malloc.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Wed May 21 20:08:11 2014 xsyann
** Last update Sun May 25 09:06:58 2014 xsyann
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

/* Return 1 if the page at address can be removed.
 * A page can be removed when 'region' is the only region
 * between address and address + PAGE_SIZE. */
int can_remove_page(unsigned long address,
                    struct region_struct *region,
                    struct region_struct *regions)
{
        struct region_struct *prev = NULL;
        struct region_struct *next = NULL;
        unsigned long prev_end, next_start;
        struct list_head *item;

        if (!list_is_first(&region->list, &regions->list)) {
                /* Find the first previous used region */
                prev = region;
                item = &region->list;
                while ((item = item->prev) &&
                       (item != &regions->list) &&
                       ((prev = list_entry(item, struct region_struct, list))->free));
                       prev_end = (unsigned long)prev->virtual_start + prev->size;
                prev_end = align_ceil(prev_end, PAGE_SHIFT);

                if (prev_end > address)
                        return 0;
        }

        if (!list_is_last(&region->list, &regions->list)) {
                /* Find the first next used region */
                next = region;
                item = &region->list;
                while ((item = item->next) &&
                       (item != &regions->list) &&
                       ((next = list_entry(item, struct region_struct, list))->free));
                next_start = (unsigned long)next->virtual_start;
                next_start = align_floor(next_start, PAGE_SHIFT);

                if (next_start < address + PAGE_SIZE)
                        return 0;
        }

        return 1;
}

/* Remove all pages in storage for region at address */
void remove_storage_pages(pid_t pid, unsigned long address)
{
        struct area_struct *area;
        struct region_struct *region;
        unsigned long start, end;

        if ((area = get_area(pid, address, &area_list)) == NULL)
                return;
        if ((region = get_region(area, address)) == NULL)
                return;

        PR_DEBUG(D_MED, "Remove region: size = %ld, pid = %d, address = %016lx",
                 region->size, area->pid, address);

        start = (unsigned long)region->virtual_start;
        end = start + region->size;
        /* Page align */
        start = align_floor(start, PAGE_SHIFT);
        end = align_ceil(end, PAGE_SHIFT);
        for (; start < end; start += PAGE_SIZE)
                if (is_stored_page(pid, start, &stored_pages))
                        if (can_remove_page(start, region, &area->regions)) {
                                PR_DEBUG(D_MAX, "Storage remove: pid = %d, address = %016lx (page %ld)",
                                         pid, start, ADDR_OFFSET(area->vma->vm_start, start));
                                storage_ops->remove(pid, start);
                                remove_stored_page(pid, start, &stored_pages);
                        }
}

/* Free area regions from storage and remove from list
 * Free buffer 'pid' if no more vma for pid. */
static void close_vma(struct vm_area_struct *vma)
{
        struct area_struct *area;
        struct region_struct *region, *tmp_region;
        unsigned long start;

        list_for_each_entry(area, &area_list.list, list) {
                if (area->vma == vma) {
                        list_for_each_entry_safe(region, tmp_region,
                                                 &area->regions.list, list) {
                                start = (unsigned long)region->virtual_start;
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
        if (res == 1) {
                buffer = kmap(page);
                if (buffer == NULL)
                        return -ENOMEM;
                res = storage_ops->save(pid, start, buffer);
                PR_DEBUG(D_MED, "Storage save: pid = %d, address = %016lx (page %ld), count = %d",
                         pid, start, ADDR_OFFSET(vma->vm_start, start), res);
                if (res < 0)
                        return res;
                if (!is_stored_page(pid, start, &stored_pages))
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
static int unmap_buffer(struct mapped_buffer *buffer)
{
        int error;

        if (is_existing_vma(buffer->vma)) {
                if ((error = copy_page_from_user(buffer->pid, buffer->start, buffer->vma)))
                        return error;
                if (remove_user_page(buffer->start, buffer->vma))
                        return -1;
                PR_DEBUG(D_MED, "Remove user page: pid = %d, address = %016lx",
                        buffer->pid, buffer->start);
                buffer->vma = NULL;
        }
        return 0;
}


/* Unmap page for pid if needed. If cache is set try to cache the current page.
 * If cache is not set, unmap buffer and cache. */
static int unmap_page(pid_t pid, int cache)
{
        int error = 0;
        struct mapped_buffer *buffer = get_buffer(pid, &buffers);

        if (buffer == NULL)
                return 0;

        if (cache)
                if (buffer->cache_vma) {
                        /* Unmap cache */
                        swap_buffer_cache(buffer);
                        return unmap_buffer(buffer);
                }
                else
                        swap_buffer_cache(buffer);
        else
        {
                if (buffer->vma) /* Unmap cache */
                        if ((error = unmap_buffer(buffer)))
                                return error;
                if (buffer->cache_vma) { /* Unmap buffer */
                        swap_buffer_cache(buffer);
                        return unmap_buffer(buffer);
                }
        }
        return 0;
}

/* Fill buffer and insert it into user address space. */
static struct page *map_page(pid_t pid, unsigned long address,
                             struct vm_area_struct *vma)
{
        int res;
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
                res = storage_ops->load(pid, address, buffer->buffer);
                PR_DEBUG(D_MED, "Storage load: pid = %d, address = %016lx (page %ld), count = %d",
                         pid, address, ADDR_OFFSET(vma->vm_start, address), res);
               if (res < 0)
                       return NULL;
        }
        else
        {
                PR_DEBUG(D_MED, "New page: pid = %d, address = %016lx (page %ld)",
                         pid, address, ADDR_OFFSET(vma->vm_start, address));
                memset(buffer->buffer, 0, PAGE_SIZE);
        }
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
        if (unmap_page(pid, 1))
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

        PR_DEBUG(D_MIN, "Close call pid = %d, vma = %016lx", current->pid, vma->vm_start);
        PR_DEBUG(D_MIN, "Close exec pid = %d, vma = %016lx", current->pid, vma->vm_start);

        /* To avoid unmapping in closed vma */
        list_for_each_entry(buffer, &buffers.list, list) {
                if (buffer->vma == vma)
                        buffer->vma = NULL;
        }

        close_vma(vma);

        /* Debug */
        dump_buffers(&buffers);
        dump_stored_pages(&stored_pages);
        dump_area_list(&area_list);

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
                PR_WARN(NETMALLOC_WARN_PAGE, address);
                return VM_FAULT_SIGBUS;
        }

        /* Debug */
        dump_buffers(&buffers);
        dump_stored_pages(&stored_pages);

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
        unmap_page(pid, 0);
        remove_storage_pages(pid, (unsigned long)ptr);
        remove_region(pid, (unsigned long)ptr, &area_list);

        /* Debug */
        dump_buffers(&buffers);
        dump_stored_pages(&stored_pages);
        dump_area_list(&area_list);

        up(&sem);
        up_write(&current->mm->mmap_sem);
        PR_DEBUG(D_MIN, "Free end %p, pid %d\n", ptr, current->pid);
}

void *generic_malloc(unsigned long size)
{
        int error;
        struct region_struct *region;
        void *start = NULL;

        PR_DEBUG(D_MIN, "Alloc call for %ld bytes in process %d (%s)",
                 size, current->pid, current->comm);

        down_write(&current->mm->mmap_sem);
        down_interruptible(&sem);
        PR_DEBUG(D_MIN, "Alloc exec for %ld bytes in process %d (%s)",
                 size, current->pid, current->comm);

        region = create_region(size, current, &area_list, &vm_ops);
        if (region == NULL)
                goto out;

        /* Unmap old page to be sure to reload data if region is created in the
           same vma page as the mapped current one. */
        if ((error = unmap_page(current->pid, 0)) < 0)
                goto out;

        /* Debug */
        dump_buffers(&buffers);
        dump_stored_pages(&stored_pages);
        dump_area_list(&area_list);

        start = region->virtual_start;

out:
        up(&sem);
        up_write(&current->mm->mmap_sem);
        PR_DEBUG(D_MIN, "Alloc end for %ld bytes in process %d (%s) - return %p\n",
                 size, current->pid, current->comm, start);

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
        /* Debug */
        dump_buffers(&buffers);
        dump_stored_pages(&stored_pages);
        dump_area_list(&area_list);

        remove_buffers(&buffers);
        remove_stored_pages(&stored_pages);
        remove_area_list(&area_list);
        storage_ops->release();
}

