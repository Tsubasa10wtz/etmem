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
 * Description: data structure used in interprocess communication.
 ******************************************************************************/
#ifndef MEMDCD_MESSAGE_H
#define MEMDCD_MESSAGE_H
#include <stdint.h>

enum memdcd_cmd_type {
    MEMDCD_CMD_MEM,
    MEMDCD_CMD_NODE,
    MEMDCD_CMD_POLICY,
};

enum SwapType {
    SWAP_TYPE_VMA_ADDR = 0xFFFFFF01,
    SWAP_TYPE_MAX
};

#define MAX_VMA_NUM 512
struct vma_addr {
    uint64_t start_addr;
    uint64_t vma_len;
};

struct vma_addr_with_count {
    struct vma_addr vma;
    int count;
};

struct swap_vma {
    enum SwapType type;
    uint64_t length;
    struct vma_addr vma_addrs[MAX_VMA_NUM];
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
    enum memdcd_cmd_type cmd_type;
    union {
        struct memory_message memory_msg;
    };
};

#endif

