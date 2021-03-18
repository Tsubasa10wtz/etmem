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
 * Description: This is a header file of the function declaration for common functions
 *              and the data structure definition for etmem project.
 ******************************************************************************/

#ifndef __ETMEM_COMMON_H__
#define __ETMEM_COMMON_H__

#include <stdint.h>
#include "etmem.h"

#define PROJECT_NAME_MAX_LEN 32
#define FILE_NAME_MAX_LEN    256
#define SOCKET_NAME_MAX_LEN  107

struct mem_proj {
    etmem_cmd cmd;
    char *proj_name;
    char *file_name;
    char *sock_name;
};

int parse_name_string(const char *val, char **name_str, size_t max_len);
int etmem_parse_check_result(int params_cnt, int argc);
void free_proj_member(struct mem_proj *proj);

#endif
