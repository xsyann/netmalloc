/*
** stored_page.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sat May 24 00:47:48 2014 xsyann
** Last update Sun May 25 05:55:13 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include "kutils.h"
#include "stored_page.h"

/* Add a stored_page */
struct stored_page *add_stored_page(pid_t pid, unsigned long start,
                                    struct stored_page *stored_pages)
{
        struct stored_page *page;

        page = kmalloc(sizeof(*page), GFP_KERNEL);
        if (page == NULL)
                return NULL;
        page->pid = pid;
        page->start = start;
        INIT_LIST_HEAD(&page->list);
        list_add_tail(&page->list, &stored_pages->list);
        return page;
}

/* Return 1 if a page is stored for pid, start */
int is_stored_page(pid_t pid, unsigned long start,
                   struct stored_page *stored_pages)
{
        struct stored_page *page;

        list_for_each_entry(page, &stored_pages->list, list)
                if (page->pid == pid && page->start == start)
                        return 1;
        return 0;
}

/* Free stored page for pid, start */
void remove_stored_page(pid_t pid, unsigned long start,
                        struct stored_page *stored_pages)
{
        struct stored_page *page;

        list_for_each_entry(page, &stored_pages->list, list) {
                if (page->pid == pid && page->start == start) {
                        list_del(&page->list);
                        kfree(page);
                        break;
                }
        }
}

/* Print all stored pages */
void dump_stored_pages(struct stored_page *stored_pages)
{
        struct stored_page *page;
        unsigned int i = 0;

        PR_DEBUG(D_STO, "Stored pages:");
        list_for_each_entry(page, &stored_pages->list, list)
                PR_DEBUG(D_STO, "- Page %d: pid = %d, address = %016lx", ++i, page->pid, page->start);
        PR_DEBUG(D_STO, "");
}

/* Init stored pages list */
void init_stored_pages(struct stored_page *stored_pages)
{
        INIT_LIST_HEAD(&stored_pages->list);
}

/* Free stored pages list */
void remove_stored_pages(struct stored_page *stored_pages)
{
        struct stored_page *page, *tmp;

        list_for_each_entry_safe(page, tmp, &stored_pages->list, list) {
                list_del(&page->list);
                kfree(page);
        }
}
