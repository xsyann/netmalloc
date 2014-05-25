/*
** stored_page.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sat May 24 00:46:14 2014 xsyann
** Last update Sun May 25 04:17:08 2014 xsyann
*/

#ifndef         __STORED_PAGE_H__
#define         __STORED_PAGE_H__

struct stored_page
{
        pid_t pid;
        unsigned long start;
        struct list_head list;
};

struct stored_page *add_stored_page(pid_t pid, unsigned long start,
                                    struct stored_page *stored_pages);
int is_stored_page(pid_t pid, unsigned long start,
                   struct stored_page *stored_pages);
void remove_stored_page(pid_t pid, unsigned long start,
                        struct stored_page *stored_pages);

void init_stored_pages(struct stored_page *stored_pages);
void remove_stored_pages(struct stored_page *stored_pages);

void dump_stored_pages(struct stored_page *stored_pages);

#endif          /* __STORED_PAGE_H__ */
