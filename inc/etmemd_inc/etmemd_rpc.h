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
 * Description: This is a header file of the function declaration for interaction function
 *              between server and client.
 ******************************************************************************/

#ifndef ETMEMD_RPC_H
#define ETMEMD_RPC_H
#include <stdbool.h>

enum cmd_type {
    OBJ_ADD = 0,
    OBJ_DEL,
    MIG_START,
    MIG_STOP,
    PROJ_SHOW,
    ENG_CMD,
};

enum rpc_decode_type {
    DECODE_STRING = 0,
    DECODE_INT,
    DECODE_END,
};

struct server_rpc_params {
    char *proj_name;
    char *file_name;
    int cmd;
    int sock_fd;
    char *eng_name;
    char *eng_cmd;
    char *task_name;
};

struct server_rpc_parser {
    const char *name;
    enum rpc_decode_type type;
    void *member_ptr;
};

void etmemd_handle_signal(void);
int etmemd_parse_rpc_port(int port);
int etmemd_parse_sock_name(const char *sock_name);
int etmemd_rpc_server(void);
bool etmemd_sock_name_set(void);
void etmemd_sock_name_free(void);
int etmemd_deal_systemctl(void);

#endif
