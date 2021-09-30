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
 * Description: handle receive buffer.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stddef.h>

#include "memdcd_process.h"
#include "memdcd_log.h"
#include "memdcd_cmd.h"

#define FILEPATH_MAX_LEN 64

static uint64_t get_process_vmsize(int pid)
{
    char statm_path[FILEPATH_MAX_LEN];
    FILE *fp = NULL;
    uint64_t vmsize;
    char error_str[ERROR_STR_MAX_LEN] = {0};

    if (snprintf(statm_path, FILEPATH_MAX_LEN, "/proc/%d/statm", pid) <= 0) {
        memdcd_log(_LOG_ERROR, "memdcd_uswap: snprintf for statm_path from pid %d failed.", pid);
        return -1;
    }
    fp = fopen(statm_path, "r");
    if (fp == NULL) {
        memdcd_log(_LOG_ERROR, "Error opening statm file %s: %s.", statm_path, error_str);
        return 0;
    }
    if (fscanf(fp, "%lu", &vmsize) <= 0) {
        memdcd_log(_LOG_ERROR, "Error reading file %s. err: %s",
            statm_path, strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return vmsize;
}

static int handle_mem_message(const struct memory_message *msg)
{
    uint64_t total_pages = get_process_vmsize(msg->pid);
    if (total_pages == 0) {
        memdcd_log(_LOG_ERROR, "Error getting vmsize of process %d.", msg->pid);
        return -1;
    } else if (total_pages < msg->vma.total_length) {
        memdcd_log(_LOG_ERROR, "Total page num of process %lu is less than incoming page num %lu.",
                   total_pages, msg->vma.total_length);
        return -1;
    }

    if (msg->vma.length / sizeof(struct vma_addr_with_count) > MAX_VMA_NUM) {
        memdcd_log(_LOG_ERROR, "Invalid message length %lu.", msg->vma.length);
        return -1;
    }

    return migrate_process_get_pages(msg->pid, &msg->vma);
}

int handle_recv_buffer(const void *buffer, int msg_len)
{
    struct memdcd_message *msg = (struct memdcd_message *)buffer;
    if (msg_len != sizeof(struct memdcd_message)) {
        memdcd_log(_LOG_ERROR, "Invalid recv message length %d.", msg_len);
        return -1;
    }
    memdcd_log(_LOG_DEBUG, "Type: %d.", msg->cmd_type);

    switch (msg->cmd_type) {
        case MEMDCD_CMD_MEM:
            return handle_mem_message(&msg->memory_msg);
        default:
            memdcd_log(_LOG_ERROR, "Invalid cmd type.");
            return -1;
    }
    return 0;
}