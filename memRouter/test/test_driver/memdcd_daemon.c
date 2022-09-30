/* *****************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: yangxin
 * Create: 2022-09-26
 * Description: memdcd_daemon.c without accept
 * **************************************************************************** */
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "memdcd_cmd.h"
#include "memdcd_log.h"
#include "memdcd_daemon.h"

#define MAX_PENDING_QUEUE_LENGTH 64
#define MAX_SOCK_PATH_LENGTH 108

static volatile sig_atomic_t g_exit_signal;

static void _set_exit_flag(int s)
{
    (void)s;
    g_exit_signal = 1;
}

static void memdcd_install_signal(void)
{
    signal(SIGINT, _set_exit_flag);
    signal(SIGTERM, _set_exit_flag);
}

static int _set_socket_option(int sock_fd)
{
    int rc;
    int buf_len = MAX_MESSAGE_LENGTH;
    struct timeval timeout = { 5, 0 };

    /* set timeout limit to socket for sending */
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
    if (rc < 0) {
        memdcd_log(_LOG_ERROR, "set send timeout for socket failed, err(%s)", strerror(errno));
        return -1;
    }

    /* set max length of buffer to recive */
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, (const char *)&buf_len, sizeof(buf_len));
    if (rc < 0) {
        memdcd_log(_LOG_ERROR, "set recive buffer length for socket failed, err(%s)", strerror(errno));
        return -1;
    }

    /* set max length of buffer to send */
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, (const char *)&buf_len, sizeof(buf_len));
    if (rc < 0) {
        memdcd_log(_LOG_ERROR, "set send buffer length for socket failed, err(%s)", strerror(errno));
        return -1;
    }

    return 0;
}

static int memdcd_server_init(const char *sock_path)
{
    int sock_fd;
    struct sockaddr_un sock_addr;
    size_t sock_len;

    memset(&sock_addr, 0, sizeof(struct sockaddr_un));

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        memdcd_log(_LOG_ERROR, "create socket for fail, error(%s)", strerror(errno));
        return -1;
    }

    sock_len = strlen(sock_path);
    if (sock_len > MAX_SOCK_PATH_LENGTH) {
        memdcd_log(_LOG_ERROR, "socket path is too long (should be shorter than 108 characters)");
        close(sock_fd);
        return -1;
    }
    sock_addr.sun_family = AF_UNIX;
    memcpy(sock_addr.sun_path, sock_path, sock_len);

    sock_addr.sun_path[0] = 0;
    sock_len += offsetof(struct sockaddr_un, sun_path);

    if (_set_socket_option(sock_fd) != 0) {
        memdcd_log(_LOG_ERROR, "set for socket fail, error(%s)", strerror(errno));
        close(sock_fd);
        return -1;
    }

    if (bind(sock_fd, (struct sockaddr *)&sock_addr, sock_len) != 0) {
        memdcd_log(_LOG_ERROR, "socket bind %s fail, error(%s)", (char *)&sock_addr.sun_path[1], strerror(errno));
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

static int check_socket_permission(int sock_fd)
{
    struct ucred cred;
    socklen_t len;
    ssize_t rc;

    len = sizeof(struct ucred);

    rc = getsockopt(sock_fd, SOL_SOCKET, SO_PEERCRED, &cred, &len);
    if (rc < 0) {
        memdcd_log(_LOG_ERROR, "getsockopt failed, err(%s)\n", strerror(errno));
        return -1;
    }

    if (cred.uid != 0 || cred.gid != 0) {
        memdcd_log(_LOG_ERROR, "Socket connect failed, need recieving from app of root privilege.\n");
        return -1;
    }

    return 0;
}

static int memdcd_accept(int sock_fd, char *recv_buf)
{
    int accp_fd = -1;
    int ret = 0;
    ssize_t rc;

    accp_fd = accept(sock_fd, NULL, NULL);
    if (accp_fd < 0) {
        memdcd_log(_LOG_ERROR, "accept message failed: %s", strerror(errno));
        return -1;
    }

    rc = check_socket_permission(accp_fd);
    if (rc != 0) {
        ret = rc;
        goto close_fd;
    }

    rc = recv(accp_fd, recv_buf, MAX_MESSAGE_LENGTH, 0);
    if (rc <= 0) {
        memdcd_log(_LOG_WARN, "socket recive from client fail: %s", strerror(errno));
        ret = -1;
        goto close_fd;
    }

    if (rc > MAX_MESSAGE_LENGTH) {
        memdcd_log(_LOG_WARN, "buffer sent to memdcd is too long, should be less than %d", MAX_MESSAGE_LENGTH);
        ret = -1;
        goto close_fd;
    }

    memdcd_log(_LOG_DEBUG, "memdcd got one connection");
    ret = rc;

close_fd:
    close(accp_fd);
    return ret;
}

void *memdcd_daemon_start(const char *sock_path)
{
    int sock_fd;
    char *recv_buf = NULL;
    int msg_len;

    if (sock_path == NULL) {
        return NULL;
    }
    g_exit_signal = 0;
    memdcd_install_signal();

    sock_fd = memdcd_server_init(sock_path);
    if (sock_fd < 0) {
        return NULL;
    }

    recv_buf = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
    if (recv_buf == NULL) {
        memdcd_log(_LOG_ERROR, "failed to alloc buffer to receive message");
        goto close_fd;
    }

    /* allow RPC_CLIENT_MAX clients to connect at the same time */
    if (listen(sock_fd, MAX_PENDING_QUEUE_LENGTH) != 0) {
        memdcd_log(_LOG_ERROR, "error listening on socket %s: %s", sock_path, strerror(errno));
        goto free_buf;
    }

    memdcd_log(_LOG_INFO, "start listening on %s", sock_path);
    free(recv_buf);
    close(sock_fd);
    return (sock_path);
free_buf:
    free(recv_buf);
close_fd:
    close(sock_fd);
    return NULL;
}
