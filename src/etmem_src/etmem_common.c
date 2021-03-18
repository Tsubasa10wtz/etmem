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
 * Description: Implementation of common functions.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include "securec.h"
#include "etmem_common.h"

int parse_name_string(const char *val, char **name_str, size_t max_len)
{
    size_t len;
    int ret;

    if (val == NULL) {
        printf("name string should not be NULL\n");
        return -EINVAL;
    }
                
    len = strlen(val) + 1;
    if (len == 1) {
        printf("name string should not be empty\n");
        return -EINVAL;
    }
    if (len > max_len) {
        printf("string is too long, it should not be larger than %lu\n", max_len);
        return -ENAMETOOLONG;
    }

    *name_str = (char *)calloc(len, sizeof(char));
    if (*name_str == NULL) {
        printf("malloc project name failed.\n");
        return -ENOMEM;
    }

    ret = strncpy_s(*name_str, len, val, len - 1);
    if (ret != EOK) {
        printf("strncpy_s project name failed.\n");
        free(*name_str);
        *name_str = NULL;
        return ret;
    }

    return 0;
}

int etmem_parse_check_result(int params_cnt, int argc)
{
    if (params_cnt > 0 && argc - 1 > params_cnt * 2) {  /* maximum number of args is limited to params_cnt * 2+1 */
        printf("warn: useless parameters passed in\n");
        return -E2BIG;
    }

    return 0;
}

void free_proj_member(struct mem_proj *proj)
{
    if (proj->proj_name != NULL) {
        free(proj->proj_name);
        proj->proj_name = NULL;
    }

    if (proj->file_name != NULL) {
        free(proj->file_name);
        proj->file_name = NULL;
    }

    if (proj->sock_name != NULL) {
        free(proj->sock_name);
        proj->sock_name = NULL;
    }
}


