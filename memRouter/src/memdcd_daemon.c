/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * etmem/memRouter licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: liruilin
 * Create: 2021-02-26
 * Description: init memdcd daemon.
 ******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

#include "memdcd_log.h"
#include "memdcd_process.h"
#include "memdcd_cmd.h"
#include "memdcd_daemon.h"

#define MAX_PENDING_QUEUE_LENGTH 64
#define MAX_SOCK_PATH_LENGTH 108

static volatile sig_atomic_t g_sock_fd;
static volatile sig_atomic_t g_exit_signal;

static void _set_exit_flag(int s)
{
    (void)s;
    g_exit_signal = 1;
    if (g_sock_fd > 0) {
        close(g_sock_fd);
    }
    g_sock_fd = -1;
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
    struct timeval timeout = {5, 0};
    char error_str[ERROR_STR_MAX_LEN] = {0};

    /* set timeout limit to socket for sending */
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
    if (rc < 0) {
        memdcd_log(_LOG_ERROR, "Set send timeout for socket failed. err: %s",
            strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        return -1;
    }

    /* set max length of buffer to recive */
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, (const char *)&buf_len, sizeof(buf_len));
    if (rc < 0) {
        memdcd_log(_LOG_ERROR, "Set recive buffer length for socket failed. err: %s",
            strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        return -1;
    }

    /* set max length of buffer to send */
    rc = setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, (const char *)&buf_len, sizeof(buf_len));
    if (rc < 0) {
        memdcd_log(_LOG_ERROR, "Set send buffer length for socket failed. err: %s",
            strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        return -1;
    }

    return 0;
}

static int memdcd_server_init(const char *sock_path)
{
    int sock_fd;
    struct sockaddr_un sock_addr;
    size_t sock_len;
    char error_str[ERROR_STR_MAX_LEN] = {0};

    memset(&sock_addr, 0, sizeof(struct sockaddr_un));

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        memdcd_log(_LOG_ERROR, "Create socket for fail. err: %s", strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        return -1;
    }

    sock_len = strlen(sock_path);
    if (sock_len >= MAX_SOCK_PATH_LENGTH) {
        memdcd_log(_LOG_ERROR, "Socket path is too long.");
        close(sock_fd);
        return -1;
    }
    sock_addr.sun_family = AF_UNIX;
    memcpy(sock_addr.sun_path, sock_path, sock_len);

    sock_addr.sun_path[0] = 0;
    sock_len += offsetof(struct sockaddr_un, sun_path);

    if (_set_socket_option(sock_fd) != 0) {
        memdcd_log(_LOG_ERROR, "Set for socket fail. err: %s", strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        close(sock_fd);
        return -1;
    }

    if (bind(sock_fd, (struct sockaddr *)&sock_addr, sock_len) != 0) {
        memdcd_log(_LOG_ERROR, "Socket bind %s fail. err: %s",
            (char *)&sock_addr.sun_path[1], strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
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
    char error_str[ERROR_STR_MAX_LEN] = {0};

    len = sizeof(struct ucred);

    rc = getsockopt(sock_fd,
                    SOL_SOCKET,
                    SO_PEERCRED,
                    &cred,
                    &len);
    if (rc < 0) {
        memdcd_log(_LOG_ERROR, "Getsockopt failed. err: %s\n", strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        return -1;
    }

    if (cred.uid != 0 || cred.gid != 0) {
        memdcd_log(_LOG_ERROR, "Socket connect failed, need recieving from app of root privilege.\n");
        return -1;
    }

    return 0;
}

static int memdcd_accept(char *recv_buf)
{
    int accp_fd = -1;
    int ret = 0;
    ssize_t rc;
    char error_str[ERROR_STR_MAX_LEN] = {0};

    accp_fd = accept(g_sock_fd, NULL, NULL);
    if (accp_fd < 0) {
        memdcd_log(_LOG_ERROR, "Accept message failed. err: %s", strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        return -1;
    }

    rc = check_socket_permission(accp_fd);
    if (rc != 0) {
        ret = rc;
        goto close_fd;
    }

    rc = recv(accp_fd, recv_buf, MAX_MESSAGE_LENGTH, 0);
    if (rc <= 0) {
        memdcd_log(_LOG_WARN, "Socket recive from client fail. err: %s", \
            strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        ret = -1;
        goto close_fd;
    }

    if (rc > MAX_MESSAGE_LENGTH) {
        memdcd_log(_LOG_WARN, "Buffer sent to memdcd is too long, should be less than %d.", MAX_MESSAGE_LENGTH);
        ret = -1;
        goto close_fd;
    }

    memdcd_log(_LOG_DEBUG, "Memdcd got one connection.");
    ret = rc;

close_fd:
    close(accp_fd);
    return ret;
}

void *memdcd_daemon_start(const char *sock_path)
{
    char *recv_buf = NULL;
    int msg_len;
    int sock_fd;
    char error_str[ERROR_STR_MAX_LEN] = {0};

    if (sock_path == NULL) {
        return NULL;
    }
    g_exit_signal = 0;
    g_sock_fd = 0;
    memdcd_install_signal();

    sock_fd = memdcd_server_init(sock_path);
    if (sock_fd < 0)
        return NULL;

    recv_buf = (char *)malloc(sizeof(char) * MAX_MESSAGE_LENGTH);
    if (recv_buf == NULL) {
        memdcd_log(_LOG_ERROR, "Failed to alloc buffer to receive message.");
        close(sock_fd);
        sock_fd = -1;
        return NULL;
    }

    /* allow RPC_CLIENT_MAX clients to connect at the same time */
    if (listen(sock_fd, MAX_PENDING_QUEUE_LENGTH) != 0) {
        memdcd_log(_LOG_ERROR, "Error listening on socket %s. err: %s",
            sock_path, strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        close(sock_fd);
        sock_fd = -1;
        goto free_buf;
    }

    if (g_sock_fd < 0) {
        close(sock_fd);
        sock_fd = -1;
        goto free_buf;
    }
    g_sock_fd = sock_fd;
    memdcd_log(_LOG_INFO, "Start listening on %s.", sock_path);
    while (g_exit_signal == 0) {
        msg_len = memdcd_accept(recv_buf);
        if (msg_len < 0) {
            memdcd_log(_LOG_ERROR, "Error accepting message. err: %s", strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
            continue;
        }
        if (handle_recv_buffer(recv_buf, msg_len) < 0)
            memdcd_log(_LOG_DEBUG, "Error handling message.");
    }
    migrate_process_exit();

    if (g_sock_fd > 0) {
        close(g_sock_fd);
        g_sock_fd = -1;
    }
free_buf:
    free(recv_buf);
    return NULL;
}
