/*
** storage.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Wed May 21 11:11:01 2014 xsyann
** Last update Wed May 21 11:35:23 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>

#include "netmalloc.h"
#include "storage.h"

static struct kernel_buffer_list *kernel_buffer_list = NULL;

/* ************************************************************ */

int network_save(unsigned long address, void *buffer)
{
        return 0;
}

int network_load(unsigned long address, void *buffer)
{
        return 0;
}

void network_release(void)
{
}

/* ************************************************************ */

/* Return kernel buffer corresponding to address.
 * If not found and create == 1, create a new one. */
static struct kernel_buffer_list *get_kernel_buffer(unsigned long address,
                                                    int create)
{
        struct kernel_buffer_list *kb = NULL;

        list_for_each_entry(kb, &kernel_buffer_list->list, list) {
                if (kb->address == address)
                        return kb;
        }
        if (create) {
                kb = kmalloc(sizeof(*kb), GFP_KERNEL);
                if (kb == NULL)
                        return NULL;
                kb->address = address;
                kb->buffer = NULL;
                INIT_LIST_HEAD(&kb->list);
                list_add_tail(&kb->list, &kernel_buffer_list->list);
        }
        return kb;
}

int kernel_save(unsigned long address, void *buffer)
{
        struct kernel_buffer_list *kb;

        /* If list doesn't exist, create one */
        if (kernel_buffer_list == NULL) {
                kernel_buffer_list =
                        kmalloc(sizeof(*kernel_buffer_list), GFP_KERNEL);
                INIT_LIST_HEAD(&kernel_buffer_list->list);
        }
        if (kernel_buffer_list == NULL)
                return -ENOMEM;

        kb = get_kernel_buffer(address, 1);
        if (kb == NULL)
                return -ENOMEM;

        PR_DEBUG("Save buffer at %016lx %p", address, buffer);

        /* If buffer doesn't exist, create it and copy */
        if (kb->buffer == NULL) {
                kb->buffer = kmemdup(buffer, PAGE_SIZE, GFP_KERNEL);
                if (kb->buffer == NULL)
                        return -ENOMEM;
        }
        else /* If buffer exists, copy into it */
                memcpy(kb->buffer, buffer, PAGE_SIZE);
        return 0;
}

int kernel_load(unsigned long address, void *buffer)
{
        struct kernel_buffer_list *kb;

        if (kernel_buffer_list != NULL) {
                kb = get_kernel_buffer(address, 0);
                if (kb && kb->buffer) {
                        memcpy(buffer, kb->buffer, PAGE_SIZE);
                        PR_DEBUG("Load buffer at %016lx %p", address, buffer);
                }
        }
        return 0;
}

void kernel_release(void)
{
        struct kernel_buffer_list *kb, *tmp;
        if (kernel_buffer_list)
        {
                list_for_each_entry_safe(kb, tmp,
                                         &kernel_buffer_list->list, list) {
                        if (kb->buffer)
                                kfree(kb->buffer);
                        list_del(&kb->list);
                        kfree(kb);
                }
                kfree(kernel_buffer_list);
        }
}
