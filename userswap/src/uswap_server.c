/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * userswap licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: liuyongqiang
 * Create: 2020-11-06
 * Description: userswap server definition.
 ******************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include "uswap_log.h"
#include "uswap_server.h"

#define MAX_LISTENQ_NUM 1
#define RESP_MSG_MAX_LEN 10
#define PATH_MAX_LEN 127


int init_socket(void)
{
    int socket_fd;
    int addrlen;
    struct sockaddr_un addr;
    char abstract_path[PATH_MAX_LEN] = {0};
    pid_t pid = getpid();

    socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        uswap_log(USWAP_LOG_ERR, "create socket failed\n");
        return -EINVAL;
    }

    bzero(&addr, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;
    snprintf(abstract_path, sizeof(abstract_path), "userswap%d.sock", pid);
    memcpy(addr.sun_path + 1, abstract_path, strlen(abstract_path) + 1);
    addrlen = sizeof(addr.sun_family) + strlen(abstract_path) + 1;
    if (bind(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
        uswap_log(USWAP_LOG_ERR, "bind socket failed\n");
        close(socket_fd);
        return -EINVAL;
    }

    listen(socket_fd, MAX_LISTENQ_NUM);

    return socket_fd;
}

static int check_socket_permission(int fd, int client_fd)
{
    struct ucred local_cred, client_cred;
    socklen_t len;
    int ret;

    len = sizeof(struct ucred);
    ret = getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &local_cred, &len);
    if (ret < 0) {
        uswap_log(USWAP_LOG_ERR, "get server sockopt failed\n");
        return ret;
    }
    ret = getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &client_cred, &len);
    if (ret < 0) {
        uswap_log(USWAP_LOG_ERR, "get client sockopt failed\n");
        return ret;
    }

    if (local_cred.uid != client_cred.uid ||
        local_cred.gid != client_cred.gid) {
        return -EPERM;
    }
    return 0;
}

int sock_handle_rec(int fd, struct swap_vma *swap_vma)
{
    int client_fd;
    int readbytes;
    struct sockaddr_un  clientun;
    socklen_t clientun_len = sizeof(clientun);

    client_fd = accept(fd, (struct sockaddr *)&clientun, &clientun_len);
    if (client_fd < 0) {
        return -EINVAL;
    }

    if (check_socket_permission(fd, client_fd) < 0) {
        close(client_fd);
        return -EPERM;
    }

    readbytes = read(client_fd, swap_vma, sizeof(struct swap_vma));
    if (readbytes <= 0) {
        close(client_fd);
        return -EINVAL;
    }

    return client_fd;
}

int sock_handle_respond(int client_fd, int result)
{
    int writebytes;
    char buff[RESP_MSG_MAX_LEN] = {0};

    if (client_fd < 0) {
        return -EINVAL;
    }

    if (result == 0) {
        snprintf(buff, sizeof(buff), "success");
    } else {
        snprintf(buff, sizeof(buff), "failed");
    }

    writebytes = write(client_fd, buff, (strlen(buff) + 1));
    if (writebytes != (strlen(buff) + 1)) {
        close(client_fd);
        return -EIO;
    }

    close(client_fd);
    return 0;
}
