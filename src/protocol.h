/*
** protocol.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sat May 24 21:51:08 2014 xsyann
** Last update Sat May 24 22:31:46 2014 xsyann
*/

#ifndef         __PROTOCOL_H__
#define         __PROTOCOL_H__

enum protocol_command {
        PROTOCOL_PUT = 1,
        PROTOCOL_GET,
        PROTOCOL_RM,
        PROTOCOL_RELEASE
};

struct protocol_header {
        enum protocol_command command;
        int pid;
        unsigned long address;
};


#endif          /* __PROTOCOL_H__ */
