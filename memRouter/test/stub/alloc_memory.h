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
 * Description: Allocate memory for memRouter Unit test
 * **************************************************************************** */
#ifndef STUB_ALLOC_MEMORY_H
#define STUB_ALLOC_MEMORY_H

#include "memdcd_message.h"

struct memdcd_message *alloc_memory(int total_len, int *msg_num);
void free_memory(struct memdcd_message *msg, int msg_num);
int simulate_uswap(int pid, struct vma_addr **recv_msg);
int detect_uswap_socket(int pid);
int format_msg(struct memdcd_message *msg, int msg_num, struct vma_addr **vma_msg);
int cmp_msg(struct vma_addr *msg1, long len1, struct vma_addr *msg2, long len2);

void dump_msg(struct vma_addr *msg, long len);
int connect_to_memdcd(void);
int send_to_memdcd(int sockfd, unsigned int pid, struct memdcd_message *msg);
int get_pid_max(void);
#endif
