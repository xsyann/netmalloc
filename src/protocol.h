/*
** protocol.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sat May 24 21:51:08 2014 xsyann
** Last update Sat May 24 22:06:01 2014 xsyann
*/

#ifndef         __PROTOCOL_H__
#define         __PROTOCOL_H__

#define PROTOCOL_PUT 1
#define PROTOCOL_GET 2
#define PROTOCOL_REMOVE 3
#define PROTOCOL_RELEASE 4

struct protocol_header {
        int command;
        int pid;
        unsigned long address;
};


#endif          /* __PROTOCOL_H__ */
