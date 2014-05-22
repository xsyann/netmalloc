/*
** kutils.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Thu May 22 21:08:54 2014 xsyann
** Last update Thu May 22 21:42:56 2014 xsyann
*/

#ifndef         __KUTILS_H__
#define         __KUTILS_H__

#define DEBUG_LEVEL (D_MIN | D_MED)

#define D_MIN 1 << 0
#define D_MED 1 << 1
#define D_MAX 1 << 2

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

#endif          /* __KUTILS_H__ */
