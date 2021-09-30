/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * etmem/memRouter licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: zhangxuzhou
 * Create: 2020-09-08
 * Description: init policy
 ******************************************************************************/

#ifndef MEMDCD_POLICY_H
#define MEMDCD_POLICY_H
#include "memdcd_process.h"

enum mem_policy_type {
    POL_TYPE_THRESHOLD,
    POL_TYPE_MAX,
};

struct mem_policy {
    enum mem_policy_type type;
    void *private;
    struct memdcd_policy_opt *opt;
};

struct memdcd_policy_opt {
    int (*init)(struct mem_policy *policy, const char *path);
    int (*parse)(const struct mem_policy *policy, int pid, struct migrate_page_list *page_list,
        struct migrate_page_list **pages_to_numa, struct migrate_page_list **pages_to_swap);
    int (*destroy)(struct mem_policy *policy);
};

struct mem_policy *get_policy(void);
int init_mem_policy(char *path);

#endif /* MEMDCD_POLICY_H */

