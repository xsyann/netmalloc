/*
** socket.h
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sat May 24 18:56:34 2014 xsyann
** Last update Sat May 24 19:41:22 2014 xsyann
*/

#ifndef         __SOCKET_H__
#define         __SOCKET_H__

#include <linux/net.h>

int socket_init(struct socket **socket, const char *host, int port);
void socket_release(struct socket *socket);

int socket_send(struct socket *socket, void *buf, size_t len);
int socket_recv(struct socket *socket, void *buf, size_t len);

#endif          /* __SOCKET_H__ */
