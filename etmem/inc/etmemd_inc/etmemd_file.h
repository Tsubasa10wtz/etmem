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
#include <glib.h>
#include "etmemd_project.h"
#include "etmemd_task.h"

#define PROJ_GROUP "project"
#define ENG_GROUP  "engine"
#define TASK_GROUP "task"

enum val_type {
    INT_VAL,
    STR_VAL,
};

struct config_item {
    char *key;
    enum val_type type;
    int (*fill)(void *obj, void *val);
    bool option;
};

int parse_file_config(GKeyFile *config, char *group_name, struct config_item *items, unsigned n, void *obj);

static inline int parse_to_int(void *val)
{
    return (int)(long long)val;
}

static inline unsigned int parse_to_uint(void *val)
{
    return (unsigned int)(long long)val;
}

static inline unsigned long parse_to_ulong(void *val)
{
    return (unsigned long)(long long)val;
}

#endif
