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
 * Description: This is a header file of the data structure definition for type and item.
 ******************************************************************************/

#ifndef ETMEMD_FILE_H
#define ETMEMD_FILE_H

#include <stdio.h>
#include "etmemd_project.h"
#include "etmemd_task.h"

struct project_item {
    char *proj_sec_name;
    int (*fill_proj_func)(struct project *proj, const char *val);
    bool optional;
    bool set;
};

struct task_item {
    char *task_sec_name;
    int (*fill_task_func)(struct task *tk, const char *val);
    bool optional;
    bool set;
};

int etmemd_fill_proj_by_conf(struct project *proj, FILE *conf_file);

#endif
