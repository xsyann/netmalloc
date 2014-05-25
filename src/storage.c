/*
** storage.c
**
** Made by xsyann
** Contact <contact@xsyann.com>
**
** Started on  Wed May 21 11:11:01 2014 xsyann
** Last update Sun May 25 03:27:14 2014 xsyann
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>

#include "kutils.h"
#include "socket.h"
#include "protocol.h"
#include "storage.h"

struct socket *socket = NULL;

int parse_address_param(const char *address, char *host, size_t size, long *port)
{
        char *dup, *save, *token;
        int ret = -1;

        dup = save = kstrdup(address, GFP_KERNEL);
        if (dup == NULL)
                return -ENOMEM;
        token = strsep(&dup, ":");
        if (token) {
                strncpy(host, token, size - 1);
                if (dup)
                        ret = kstrtol(dup, 10, port);
        }

        kfree(save);
        return ret;
}

int network_init(void *param)
{
        char host[16];
        long port;
        int error;

        memset(host, 0, 16);
        if ((error = parse_address_param(param, host, 16, &port)))
                return error;
        if ((error = socket_init(&socket, host, port)))
                return error;

        return 0;
}

int network_load(pid_t pid, unsigned long address, void *buffer)
{
        int size = 0;
        struct protocol_header protocol_header = {
                .command = PROTOCOL_GET,
                .pid = pid,
                .address = address,
        };

        size = socket_send(socket, &protocol_header, sizeof(protocol_header));
        if (size == sizeof(protocol_header))
                size = socket_recv(socket, buffer, PAGE_SIZE);

        return size;
}

int network_save(pid_t pid, unsigned long address, void *buffer)
{
        int size = 0;
        struct protocol_header protocol_header = {
                .command = PROTOCOL_PUT,
                .pid = pid,
                .address = address,
        };

        size = socket_send(socket, &protocol_header, sizeof(protocol_header));
        if (size == sizeof(protocol_header))
                size = socket_send(socket, buffer, PAGE_SIZE);

        return size;
}

void network_remove(pid_t pid, unsigned long address)
{
        int size = 0;
        struct protocol_header protocol_header = {
                .command = PROTOCOL_RM,
                .pid = pid,
                .address = address,
        };
        size = socket_send(socket, &protocol_header, sizeof(protocol_header));
}

void network_release(void)
{
        int size = 0;
        struct protocol_header protocol_header = {
                .command = PROTOCOL_RELEASE,
                .pid = 0,
                .address = 0,
        };

        size = socket_send(socket, &protocol_header, sizeof(protocol_header));

        socket_release(socket);
}

/* ************************************************************ */

static struct kernel_buffer_list *kernel_buffer_list = NULL;

static void dump_kernel_buffer_list(void)
{
        struct kernel_buffer_list *kb = NULL;

        if (kernel_buffer_list->list.prev == &kernel_buffer_list->list) {
                PR_DEBUG(D_MED, "Kernel Storage buffers : <empty>");
                return;
        }
        PR_DEBUG(D_MED, "Kernel Storage buffers :");
        list_for_each_entry(kb, &kernel_buffer_list->list, list) {
                PR_DEBUG(D_MED, "  %016lx, pid = %d",
                         kb->address, kb->pid);
        }
}
/* Return kernel buffer corresponding to pid, address.
 * If not found and create is set, create a new one. */
static struct kernel_buffer_list *get_kernel_buffer(pid_t pid, unsigned int address,
                                                    int create)
{
        struct kernel_buffer_list *kb = NULL;

        list_for_each_entry(kb, &kernel_buffer_list->list, list)
                if (kb->address == address && kb->pid == pid)
                        return kb;
        kb = NULL;
        if (create) {
                kb = kzalloc(sizeof(*kb), GFP_KERNEL);
                if (kb == NULL)
                        return NULL;
                kb->pid = pid;
                kb->buffer = NULL;
                INIT_LIST_HEAD(&kb->list);
                list_add_tail(&kb->list, &kernel_buffer_list->list);
        }
        return kb;
}

int kernel_init(void *param)
{
        kernel_buffer_list = kmalloc(sizeof(*kernel_buffer_list), GFP_KERNEL);
        if (kernel_buffer_list == NULL)
                return -ENOMEM;
        INIT_LIST_HEAD(&kernel_buffer_list->list);

        return 0;
}

int kernel_save(pid_t pid, unsigned long address, void *buffer)
{
        struct kernel_buffer_list *kb;

        kb = get_kernel_buffer(pid, address, 1);
        kb->address = address;
        if (kb == NULL)
                return -ENOMEM;

        PR_DEBUG(D_MED, "Save buffer at %016lx %p, pid = %d",
                 address, buffer, pid);

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

int kernel_load(pid_t pid, unsigned long address, void *buffer)
{
        struct kernel_buffer_list *kb = NULL;

        if (kernel_buffer_list != NULL) {
                kb = get_kernel_buffer(pid, address, 0);
                if (kb && kb->buffer) {
                        memcpy(buffer, kb->buffer, PAGE_SIZE);
                        PR_DEBUG(D_MED, "Load buffer at %016lx %p, pid = %d",
                                 address, buffer, pid);
                }
                else
                        memset(buffer, 0, PAGE_SIZE);
        }
        return 0;
}

void kernel_remove(pid_t pid, unsigned long address)
{
        struct kernel_buffer_list *kb, *tmp;
        if (kernel_buffer_list) {
                list_for_each_entry_safe(kb, tmp,
                                         &kernel_buffer_list->list, list) {
                        if (kb->address == address && kb->pid == pid) {
                                if (kb->buffer)
                                        kfree(kb->buffer);
                                list_del(&kb->list);
                                kfree(kb);
                        }
                }
                dump_kernel_buffer_list();
        }
}

void kernel_release(void)
{
        struct kernel_buffer_list *kb, *tmp;
        if (kernel_buffer_list) {
                dump_kernel_buffer_list();
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
