/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * etmem/memRouter licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: zhangxuzhou
 * Create: 2020-09-18
 * Description: engines migrating memory pages
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stddef.h>

#include "memdcd_policy.h"
#include "memdcd_process.h"
#include "memdcd_log.h"
#include "memdcd_migrate.h"

#define CLIENT_RECV_DEFAULT_TIME 10 // default 10s
#define RESP_MSG_MAX_LEN 10
#define FILEPATH_MAX_LEN 64

static int uswap_init_connection(int server_pid, time_t recv_timeout)
{
    socklen_t addrlen;
    struct timeval timeout;
    struct sockaddr_un addr;
    char error_str[ERROR_STR_MAX_LEN] = {0};

    int socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        memdcd_log(_LOG_ERROR, "memdcd_uswap: Error opening socket.");
        return -1;
    }

    /* set recv timeout */
    timeout.tv_sec = recv_timeout;
    timeout.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout, sizeof(struct timeval)) != 0) {
        memdcd_log(_LOG_ERROR, "memdcd_uswap: Setsockopt set recv timeout failed!");
        close(socket_fd);
        return -1;
    }

    bzero(&addr, sizeof(struct sockaddr_un));
    /* UNIX domain Socket abstract namespace */
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;

    if (snprintf(addr.sun_path + 1, FILEPATH_MAX_LEN, "userswap%d.sock", server_pid) <= 0) {
        memdcd_log(_LOG_ERROR, "memdcd_uswap: Snprintf_s for abtract_path from pid %d failed.", server_pid);
        close(socket_fd);
        return -1;
    }

    addrlen = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path + 1) + 1;
    if (connect(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
        memdcd_log(_LOG_ERROR, "memdcd_uswap: Connect failed. err: %s! ", \
            strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

static int uswap_send_data(int client_fd, int server_pid, const struct swap_vma *swap_vma)
{
    int ret = 0;
    int read_bytes;
    int write_bytes;
    int in_datalen;
    char buff[RESP_MSG_MAX_LEN] = {0};
    int len = strlen("success");

    in_datalen = swap_vma->length + sizeof(int) + sizeof(long);
    write_bytes = write(client_fd, swap_vma, in_datalen);
    if (write_bytes <= 0) {
        memdcd_log(_LOG_DEBUG, "memdcd_uswap: Write to pid %d server, bytes: %d.", server_pid, write_bytes);
        return -1;
    }

    read_bytes = read(client_fd, buff, RESP_MSG_MAX_LEN - 1);
    if (read_bytes >= len && strncmp(buff, "success", len) == 0) {
        memdcd_log(_LOG_DEBUG, "memdcd_uswap: Recv respond success.");
    } else {
        memdcd_log(_LOG_DEBUG, "memdcd_uswap: Recv respond failed.");
        ret = -1;
    }

    return ret;
}

static int min(int a, int b)
{
    return a < b ? a : b;
}

int send_to_userswap(int pid, const struct migrate_page_list *page_list)
{
    int ret = 0;
    struct swap_vma *swap_vma = NULL;
    int client_fd;
    uint64_t i, rest, size;
    uint64_t dst;
    if (page_list->pages == NULL) {
        memdcd_log(_LOG_WARN, "memdcd_uswap: Mig_addrs NULL.");
        return 0;
    }
    memdcd_log(_LOG_INFO, "Send %lu addresses to userswap: pid %d.", page_list->length, pid);
    swap_vma = (struct swap_vma *)malloc(sizeof(struct swap_vma));
    if (swap_vma == NULL) {
        memdcd_log(_LOG_WARN, "memdcd_uswap: Malloc for swap vma failed.");
        return -ENOMEM;
    }
    swap_vma->type = SWAP_TYPE_VMA_ADDR;
    rest = page_list->length;
    while (rest > 0) {
        client_fd = uswap_init_connection(pid, CLIENT_RECV_DEFAULT_TIME);
        if (client_fd < 0) {
            ret = -1;
            break;
        }
        size = min(rest, MAX_VMA_NUM);

        swap_vma->length = size * sizeof(struct vma_addr);
        for (i = 0; i < size; i++) {
            dst = page_list->length - rest + i;
            swap_vma->vma_addrs[i].start_addr = page_list->pages[dst].addr;
            swap_vma->vma_addrs[i].vma_len = page_list->pages[dst].length;
        }
        if (uswap_send_data(client_fd, pid, swap_vma) != 0)
            ret = -1;
        close(client_fd);
        rest -= size;
    }

    free(swap_vma);
    return ret;
}