/*
** kutils.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Thu May 22 21:08:54 2014 xsyann
** Last update Sun May 25 06:05:26 2014 xsyann
*/

#ifndef         __KUTILS_H__
#define         __KUTILS_H__

#define DEBUG_LEVEL (D_MIN | D_MED | D_MAX | D_AREA | D_BUF | D_STO)

#define D_MIN 1 << 0
#define D_MED 1 << 1
#define D_MAX 1 << 2

#define D_AREA 1 << 3 /* Area list */
#define D_BUF 1 << 4 /* Buffer list */
#define D_STO 1 << 5 /* Stored pages list */


#define MODNAME KBUILD_MODNAME

#define PR_WARN(ERR, ...)                                               \
        (printk(KERN_WARNING "%s: error: " ERR "\n", MODNAME, ##__VA_ARGS__))

#define PR_INFO(INFO, ...)                                              \
        (printk(KERN_INFO "%s: " INFO "\n", MODNAME, ##__VA_ARGS__))

#ifdef DEBUG_LEVEL
#define PR_DEBUG(LEVEL, DEBUG, ...)                                     \
        if (DEBUG_LEVEL & LEVEL) {                                      \
                (printk(KERN_INFO "%s: " DEBUG "\n", MODNAME, ##__VA_ARGS__)); }
#else
#define PR_DEBUG(LEVEL, INFO, ...)
#endif

static inline unsigned long align_floor(unsigned long x, unsigned long shift)
{
        return (x % (1 << shift)) ? (x >> shift << shift) : x;
}

static inline unsigned long align_ceil(unsigned long x, unsigned long shift)
{
        return (x % (1 << shift)) ? align_floor(x + (1 << shift), shift) : x;
}

#define ADDR_OFFSET(START, ADDR) ((ADDR) - (START)) >> PAGE_SHIFT

/* List */

static inline int list_is_first(const struct list_head *list, const struct list_head *head)
{
        return head->next == list;
}

#define list_last_entry(head, type, member) \
        list_entry((head)->prev, type, member)

#endif          /* __KUTILS_H__ */
