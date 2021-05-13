/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: louhongxiang
 * Create: 2019-12-10
 * Description: Interaction API between server and client.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include "securec.h"
#include "etmem_rpc.h"

#define ETMEM_CLIENT_REGISTER "proj_name %s file_name %s cmd %d eng_name %s eng_cmd %s task_name %s"

#define INT_MAX_LEN 9
#define ETMEM_RPC_RECV_BUF_LEN 512
#define ETMEM_RPC_SEND_BUF_LEN 512
#define ETMEM_RPC_CONN_TIMEOUT 10

#define SUCCESS_CHAR (0xff)
#define FAIL_CHAR (0xfe)

static int etmem_client_conn(const struct mem_proj *proj, int sockfd)
{
    struct sockaddr_un svr_addr;
    struct timeval timeout = {ETMEM_RPC_CONN_TIMEOUT, 0};
    int ret;

    ret = memset_s(&svr_addr, sizeof(struct sockaddr_un), 0, sizeof(struct sockaddr_un));
    if (ret != EOK) {
        printf("memset_s sockaddr_in failed.\n");
        return ret;
    }

    /* set timeout for client socket send */
    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
    if (ret < 0) {
        printf("set timeout for etmem client socket send fail\n");
        return ret;
    }

    /* set timeout for client socket receive */
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    if (ret < 0) {
        printf("set timeout for etmem client socket receive fail\n");
        return ret;
    }

    ret = sprintf_s(svr_addr.sun_path, sizeof(svr_addr.sun_path),
                    "@%s", proj->sock_name);
    if (ret == -1) {
        printf("set for sun_path fail\n");
        return ret;
    }
    svr_addr.sun_family = AF_UNIX;
    svr_addr.sun_path[0] = 0;

    if (connect(sockfd, (struct sockaddr *)&svr_addr,
                offsetof(struct sockaddr_un, sun_path) + strlen(proj->sock_name) + 1) < 0) {
        printf("etmem connect to server failed: %s\n", strerror(errno));
        return errno;
    }

    return 0;
}

static size_t etmem_client_get_str_len(const struct mem_proj *proj)
{
    size_t total_len;

    total_len = strlen(ETMEM_CLIENT_REGISTER) + INT_MAX_LEN + 1;
    if (proj->proj_name != NULL) {
        total_len += strlen(proj->proj_name);
    }
    if (proj->file_name != NULL) {
        total_len += strlen(proj->file_name);
    }
    if (proj->eng_name != NULL) {
        total_len += strlen(proj->eng_name);
    }
    if (proj->eng_cmd != NULL) {
        total_len += strlen(proj->eng_cmd);
    }
    if (proj->task_name != NULL) {
        total_len += strlen(proj->task_name);
    }

    return total_len;
}

static int etmem_client_send(const struct mem_proj *proj, int sockfd)
{
    size_t total_len;
    size_t reg_cmd_len;
    char *reg_cmd = NULL;
    int ret = -1;

    total_len = etmem_client_get_str_len(proj);

    reg_cmd = (char *)calloc(total_len, sizeof(char));
    if (reg_cmd == NULL) {
        printf("malloc register command failed.\n");
        return -1;
    }

    if (snprintf_s(reg_cmd, total_len, total_len - 1, ETMEM_CLIENT_REGISTER,
                   proj->proj_name == NULL ? "-" : proj->proj_name,
                   proj->file_name == NULL ? "-" : proj->file_name,
                   proj->cmd,
                   proj->eng_name == NULL ? "-" : proj->eng_name,
                   proj->eng_cmd == NULL ? "-" : proj->eng_cmd,
                   proj->task_name == NULL ? "-" : proj->task_name) <= 0) {
        printf("snprintf_s failed.\n");
        goto EXIT;
    }

    reg_cmd_len = strlen(reg_cmd);
    /* in case that the message send from client to server is too long */
    if (reg_cmd_len > ETMEM_RPC_SEND_BUF_LEN) {
        printf("send message %s too long, should not longer than %d\n",
               reg_cmd, ETMEM_RPC_SEND_BUF_LEN);
        goto EXIT;
    }

    if (send(sockfd, reg_cmd, reg_cmd_len, 0) < 0) {
        printf("send failed: %s\n", strerror(errno));
        goto EXIT;
    }
    ret = 0;

EXIT:
    free(reg_cmd);
    return ret;
}

static int etmem_client_recv(int sockfd)
{
    ssize_t recv_size;
    int ret = -1;
    char *recv_msg = NULL;
    uint8_t *recv_buf = NULL;
    size_t recv_len = ETMEM_RPC_RECV_BUF_LEN;
    bool done = false;

    recv_buf = (uint8_t *)calloc(recv_len, sizeof(uint8_t));
    if (recv_buf == NULL) {
        printf("malloc receive buffer failed.\n");
        return -1;
    }

    while (!done) {
        recv_size = recv(sockfd, recv_buf, recv_len - 1, 0);
        if (recv_size < 0) {
            printf("recv failed: %s\n", strerror(errno));
            goto EXIT;
        }
        if (recv_size == 0) {
            printf("connection closed by peer\n");
            goto EXIT;
        }

        recv_msg = (char *)recv_buf;
        recv_msg[recv_size] = '\0';

        // check and erase finish flag
        switch (recv_msg[recv_size - 1]) {
            case (char)SUCCESS_CHAR:
                ret = 0;
                done = true;
                recv_msg[recv_size - 1] = '\0';
                break;
            case (char)FAIL_CHAR:
                done = true;
                recv_msg[recv_size - 1] = '\n';
                break;
            default:
                break;
        }

        printf("%s", recv_msg);
    }

EXIT:
    free(recv_buf);
    return ret;
}

int etmem_rpc_client(const struct mem_proj *proj)
{
    int sockfd = -1;
    int ret;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("create socket failed.\n");
        return errno;
    }

    ret = etmem_client_conn(proj, sockfd);
    if (ret != 0) {
        printf("etmem_client_conn failed.\n");
        goto EXIT;
    }

    if (etmem_client_send(proj, sockfd) != 0) {
        printf("etmem_client_send failed.\n");
        ret = -EPERM;
        goto EXIT;
    }

    ret = etmem_client_recv(sockfd);
    if (ret < 0) {
        printf("etmem_client_recv failed.\n");
        ret = -ECOMM;
        goto EXIT;
    }

EXIT:
    close(sockfd);
    return ret;
}
