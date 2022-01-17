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
 * Author: Yangxin
 * Create: 2021-04-05
 * Description: API of memdcd engine.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "securec.h"
#include "etmemd_log.h"
#include "etmemd_common.h"
#include "etmemd_engine.h"
#include "etmemd_scan.h"
#include "etmemd_migrate.h"
#include "etmemd_pool_adapter.h"
#include "etmemd_file.h"
#include "etmemd_memdcd.h"

#define MAX_VMA_NUM 512
#define RESP_MSG_MAX_LEN 10
#define CLIENT_RECV_DEFAULT_TIME 10

enum MEMDCD_CMD_TYPE {
    MEMDCD_CMD_MEM = 0
};

enum SwapType {
    SWAP_TYPE_VMA_ADDR = 0xFFFFFF01,
    SWAP_TYPE_MAX
};

struct vma_addr {
    uint64_t start_addr;
    uint64_t vma_len;
};

struct vma_addr_with_count {
    struct vma_addr vma;
    int count;
};

enum MEMDCD_MESSAGE_STATUS {
    MEMDCD_SEND_START,
    MEMDCD_SEND_PROCESS,
    MEMDCD_SEND_END,
};

struct swap_vma_with_count {
    enum SwapType type;
    uint64_t length;
    uint64_t total_length;
    enum MEMDCD_MESSAGE_STATUS status;
    struct vma_addr_with_count vma_addrs[MAX_VMA_NUM];
};

struct memory_message {
    int pid;
    uint32_t enable_uswap;
    struct swap_vma_with_count vma;
};

struct memdcd_message {
    enum MEMDCD_CMD_TYPE cmd_type;
    union {
        struct memory_message memory_msg;
    };
};

static int memdcd_connection_init(time_t tm_out, const char sock_path[])
{
    struct sockaddr_un addr;
    int len;

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0){
        etmemd_log(ETMEMD_LOG_ERR, "new socket for memdcd error.");
        return -1;
    }

    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

    if (memset_s(&addr, sizeof(struct sockaddr_un),
                 0, sizeof(struct sockaddr_un)) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "clear addr failed\n");
        goto err_out;
    }

    addr.sun_family = AF_UNIX;
    if (memcpy_s(addr.sun_path, sizeof(addr.sun_path),
                 sock_path, strlen(sock_path)) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "copy for memdcd server path to addr fail, error(%s)\n",
                   strerror(errno));
        goto err_out;
    }

    /* note, the below two line **MUST** maintain the order */
    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);
    addr.sun_path[0] = '\0';
    if (connect(sockfd, (struct sockaddr *)&addr, len) < 0){
        etmemd_log(ETMEMD_LOG_ERR, "connect memdcd failed\n");
        goto err_out;
    }

    return sockfd;

err_out:
    close(sockfd);
    return -1;
}

static int send_data_to_memdcd(unsigned int pid, struct memdcd_message *msg, const char sock_path[])
{
    int client_fd;
    int ret = 0;
    int read_bytes;
    int write_bytes;
    char buff[RESP_MSG_MAX_LEN] = {0};
    time_t recv_timeout = CLIENT_RECV_DEFAULT_TIME;

    client_fd = memdcd_connection_init(recv_timeout, sock_path);
    if (client_fd < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "%s: connect error %d.\n", __func__, client_fd);
        return -1;
    }
    write_bytes = write(client_fd, msg, sizeof(struct memdcd_message));
    if (write_bytes <= 0) {
        etmemd_log(ETMEMD_LOG_DEBUG, "etmemd_socket: send to memdcd for pid %u, bytes: %d\n",
            pid, write_bytes);
        ret = -1;
        goto CLOSE_SOCK;
    }

    read_bytes = read(client_fd, buff, RESP_MSG_MAX_LEN);
    if (read_bytes > 0) {
        if (strcmp(buff, "success") == 0) {
            etmemd_log(ETMEMD_LOG_INFO, "etmemd_socket: recv respond success.\n");
        } else {
            etmemd_log(ETMEMD_LOG_ERR, "etmemd_socket: recv respond failed.\n");
            ret = -1;
        }
    }

CLOSE_SOCK:
    close(client_fd);

    return ret;
}

static int memdcd_do_migrate(unsigned int pid, struct page_refs *page_refs_list, const char sock_path[])
{
    int count = 0, total_count = 0;
    int ret = 0;
    struct swap_vma_with_count *swap_vma = NULL;
    struct page_refs *page_refs = page_refs_list;
    struct memdcd_message *msg;

    if (page_refs_list == NULL) {
        /* do nothing */
        return 0;
    }

    while (page_refs != NULL) {
        page_refs = page_refs->next;
        total_count++;
    }
    page_refs = page_refs_list;

    msg = (struct memdcd_message *)calloc(1, sizeof(struct memdcd_message));
    if (msg == NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "memigd_socket: malloc for swap vma failed. \n");
        return -1;
    }

    msg->cmd_type = MEMDCD_CMD_MEM;
    msg->memory_msg.pid = pid;
    msg->memory_msg.enable_uswap = true;
    msg->memory_msg.vma.status = MEMDCD_SEND_START;

    swap_vma = &(msg->memory_msg.vma);
    swap_vma->type = SWAP_TYPE_VMA_ADDR;
    swap_vma->total_length = total_count;

    while (page_refs != NULL) {
        swap_vma->vma_addrs[count].vma.start_addr = page_refs->addr;
        swap_vma->vma_addrs[count].vma.vma_len = page_type_to_size(page_refs->type);
        swap_vma->vma_addrs[count].count = page_refs->count;
        count++;
        page_refs = page_refs->next;

        if (count < MAX_VMA_NUM) {
            continue;
        }
        if (page_refs == NULL) {
            break;
        }
        swap_vma->length = count * sizeof(struct vma_addr_with_count);
        if (send_data_to_memdcd(pid, msg, sock_path) != 0) {
            ret = -1;
            goto FREE_SWAP;
        }
        count = 0;
        msg->memory_msg.vma.status = MEMDCD_SEND_PROCESS;
        if (memset_s(swap_vma->vma_addrs, sizeof(swap_vma->vma_addrs),
                    0, sizeof(swap_vma->vma_addrs)) != EOK) {
            etmemd_log(ETMEMD_LOG_ERR, "clear swap_vma failed\n");
            ret = -1;
            goto FREE_SWAP;
        }
    }

    if (msg->memory_msg.vma.status != MEMDCD_SEND_START)
        msg->memory_msg.vma.status = MEMDCD_SEND_END;
    swap_vma->length = count * sizeof(struct vma_addr_with_count);
    if (send_data_to_memdcd(pid, msg, sock_path) != 0) {
        ret = -1;
    }

FREE_SWAP:
    free(msg);
    return ret;
}

static struct page_refs *memdcd_do_scan(const struct task_pid *tpid, const struct task *tk)
{
    int i = 0;
    struct vmas *vmas = NULL;
    struct page_refs *page_refs = NULL;
    int ret = 0;
    char pid[PID_STR_MAX_LEN] = {0};
    char *us = "us";
    struct page_scan *page_scan = NULL;

    if(tpid == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "task pid is null\n");
        return NULL;
    }

    if (tk == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "task struct is null for pid %u\n", tpid->pid);
        return NULL;
    }

    page_scan = (struct page_scan *)tk->eng->proj->scan_param;

    if (snprintf_s(pid, PID_STR_MAX_LEN, PID_STR_MAX_LEN - 1, "%u", tpid->pid) <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf pid fail %u", tpid->pid);
        return NULL;
    }
    /* get vmas of target pid first. */
    vmas = get_vmas_with_flags(pid, &us, 1, true);
    if (vmas == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get vmas for %s fail\n", pid);
        return NULL;
    }

    /* loop for scanning idle_pages to get result of memory access. */
    for (i = 0; i < page_scan->loop; i++) {
        ret = get_page_refs(vmas, pid, &page_refs, NULL, 0);
        if (ret != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "scan operation failed\n");
            /* free page_refs nodes already exist */
            etmemd_free_page_refs(page_refs);
            page_refs = NULL;
            break;
        }
        sleep((unsigned)page_scan->sleep);
    }

    free_vmas(vmas);

    return page_refs;
}

static void *memdcd_executor(void *arg)
{
    struct task_pid *tk_pid = (struct task_pid *)arg;
    struct memdcd_params *memdcd_params = (struct memdcd_params *)(tk_pid->tk->params);
    struct page_refs *page_refs = NULL;

    /* register cleanup function in case of unexpected cancellation detected,
     * and register for memory_grade first, because it needs to clean after page_refs is cleaned */
    pthread_cleanup_push(clean_page_refs_unexpected, &page_refs);
    page_refs = memdcd_do_scan(tk_pid, tk_pid->tk);
    if (page_refs != NULL) {
        if (memdcd_do_migrate(tk_pid->pid, page_refs, memdcd_params->memdcd_socket) != 0) {
            etmemd_log(ETMEMD_LOG_WARN, "memdcd migrate for pid %u fail\n", tk_pid->pid);
        }
    }

    /* no need to use page_refs any longer.
     * pop the cleanup function with parameter 1, because the items in page_refs list will be moved
     * into the at least on list of memory_grade after polidy function called if no problems happened,
     * but mig_policy_func() may fails to move page_refs in rare cases.
     * It will do nothing if page_refs is NULL */
    pthread_cleanup_pop(1);

    return NULL;
}

static int fill_task_sock_path(void *obj, void *val)
{
    const char *default_path = "@_memdcd.server";

    struct memdcd_params *params = (struct memdcd_params *)obj;
    char *sock_path = (char *)val;

    if(strcmp(sock_path, "-") == 0) {
        if (strncpy_s(params->memdcd_socket, MAX_SOCK_PATH_LENGTH, default_path, strlen(default_path)) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "strncpy for memdcd_socket fail");
            return -1;
        }
    } else {
        if (strncpy_s(params->memdcd_socket, MAX_SOCK_PATH_LENGTH, sock_path, strlen(sock_path)) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "strncpy for memdcd_socket fail");
            return -1;
        }
    }

    return 0;
}

static struct config_item g_memdcd_task_config_items[] = {
    {"Sock", STR_VAL, fill_task_sock_path, false},
};

static int memdcd_fill_task(GKeyFile *config, struct task *tk)
{
    struct memdcd_params *params = calloc(1, sizeof(struct memdcd_params));
    if (params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc memdcd param fail\n");
        return -1;
    }

    memset_s(params->memdcd_socket, MAX_SOCK_PATH_LENGTH, 0, MAX_SOCK_PATH_LENGTH);

    if (parse_file_config(config, TASK_GROUP, g_memdcd_task_config_items, ARRAY_SIZE(g_memdcd_task_config_items),
                          (void *)params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "memdcd fill task fail\n");
        goto free_params;
    }

    if (strlen(params->memdcd_socket) >= MAX_SOCK_PATH_LENGTH) {
        etmemd_log(ETMEMD_LOG_ERR, "length of engine param Sock must less than 108.\n");
        goto free_params;
    }

    tk->params = params;
    return 0;

free_params:
    free(params);
    return -1;
}

static void memdcd_clear_task(struct task *tk)
{
    free(tk->params);
    tk->params = NULL;
}

static int memdcd_start_task(struct engine *eng, struct task *tk)
{
    struct memdcd_params *params = tk->params;

    params->executor = malloc(sizeof(struct task_executor));
    if (params->executor == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "memdcd alloc memory for task_executor fail\n");
        return -1;
    }

    params->executor->tk = tk;
    params->executor->func = memdcd_executor;
    if (start_threadpool_work(params->executor) != 0) {
        free(params->executor);
        params->executor = NULL;
        etmemd_log(ETMEMD_LOG_ERR, "memdcd start task executor fail\n");
        return -1;
    }

    return 0;
}

static void memdcd_stop_task(struct engine *eng, struct task *tk)
{
    struct memdcd_params *params = tk->params;

    stop_and_delete_threadpool_work(tk);
    free(params->executor);
    params->executor = NULL;
}

struct engine_ops g_memdcd_eng_ops = {
    .fill_eng_params = NULL,
    .clear_eng_params = NULL,
    .fill_task_params = memdcd_fill_task,
    .clear_task_params = memdcd_clear_task,
    .start_task = memdcd_start_task,
    .stop_task = memdcd_stop_task,
    .alloc_pid_params = NULL,
    .free_pid_params = NULL,
    .eng_mgt_func = NULL,
};

int fill_engine_type_memdcd(struct engine *eng, GKeyFile *config)
{
    eng->ops = &g_memdcd_eng_ops;
    eng->engine_type = MEMDCD_ENGINE;
    eng->name = "memdcd";
    return 0;
}
