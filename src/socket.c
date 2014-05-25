/*
** socket.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Sat May 24 18:57:54 2014 xsyann
** Last update Sun May 25 03:27:24 2014 xsyann
*/

#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/unistd.h>
#include <asm/uaccess.h>

#include "kutils.h"
#include "socket.h"

static unsigned int inet_addr(const char *str)
{
        int a, b, c, d;
        char arr[4];
        sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d);
        arr[0] = a; arr[1] = b; arr[2] = c; arr[3] = d;
        return *(unsigned int *)arr;
}

static void socket_prepare(struct msghdr *msg, struct iovec *iov,
                            void *buf, size_t len)
{
        msg->msg_control = NULL;
        msg->msg_controllen = 0;
        msg->msg_flags = 0;
        msg->msg_name = 0;
        msg->msg_namelen = 0;
        msg->msg_iov = iov;
        msg->msg_iovlen = 1;

        iov->iov_base = buf;
        iov->iov_len = len;
}

int socket_send(struct socket *socket, void *buf, size_t len)
{
        struct msghdr msg;
        struct iovec iov;
        mm_segment_t oldfs;
        int size = 0;

        if (socket == NULL)
                return -1;

        socket_prepare(&msg, &iov, buf, len);

        oldfs = get_fs();
        set_fs(KERNEL_DS);
        size = sock_sendmsg(socket, &msg, len);
        set_fs(oldfs);

        return size;
}

int socket_recv(struct socket *socket, void *buf, size_t len)
{
        struct msghdr msg;
        struct iovec iov;
        mm_segment_t oldfs;
        int size = 0;

        if (socket == NULL)
                return -1;

        socket_prepare(&msg, &iov, buf, len);

        oldfs = get_fs();
        set_fs(KERNEL_DS);
        size = sock_recvmsg(socket, &msg, len, msg.msg_flags);
        set_fs(oldfs);

        return size;
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
        if (socket != NULL)
                socket->ops->release(socket);
}
