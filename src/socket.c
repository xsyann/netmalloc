/*
** socket.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sat May 24 18:57:54 2014 xsyann
** Last update Sat May 24 19:02:16 2014 xsyann
*/

#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>

#include "socket.h"

unsigned int inet_addr(const char *str)
{
        int a, b, c, d;
        char arr[4];
        sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d);
        arr[0] = a; arr[1] = b; arr[2] = c; arr[3] = d;
        return *(unsigned int *)arr;
}

int socket_init(struct socket **socket, const char *host, int port)
{
        struct sockaddr_in server;
        int error;

        if ((error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, socket)))
                return error;

        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = inet_addr(host);

        if ((error = (*socket)->ops->connect(*socket, (struct sockaddr *)&server,
                                             sizeof(server), O_RDWR))) {
                (*socket)->ops->release(*socket);
                return error;
        }
        return 0;
}

void socket_release(struct socket *socket)
{
        socket->ops->release(socket);
}
