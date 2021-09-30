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
 * Description: userswap server interface
 ******************************************************************************/

#ifndef __USWAP_SERVER_H__
#define __USWAP_SERVER_H__

#define MAX_VMA_NUM 512

enum swap_type {
    SWAP_TYPE_VMA_ADDR = 0xFFFFFF01,
    SWAP_TYPE_MAX
};

struct vma_addr {
    unsigned long start_addr;
    unsigned long vma_len;
};

struct swap_vma {
    enum swap_type type;
    unsigned long length;
    struct vma_addr vma_addrs[MAX_VMA_NUM];
};

int init_socket(void);
int sock_handle_rec(int fd, struct swap_vma *swap_vma);
int sock_handle_respond(int client_fd, int result);
#endif
