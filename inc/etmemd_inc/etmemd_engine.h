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
 * Description: This is a header file of the data structure definition for etmem engine.
 ******************************************************************************/

#ifndef ETMEMD_ENGINE_H
#define ETMEMD_ENGINE_H

#include <glib.h>
#include "etmemd.h"
#include "etmemd_task.h"

enum eng_type {
    SLIDE_ENGINE = 0,
    CSLIDE_ENGINE,
    DYNAMIC_FB_ENGINE,
    HISTORICAL_FB_ENGINE,
    THIRDPARTY_ENGINE,
    ENGINE_TYPE_CNT,
};

/*
 * engine struct
 * */
struct engine {
    int engine_type;            /* engine type used for elimination strategy */
    char *name;
    void *params;               /* point to engine parameter struct */
    struct project *proj;
    struct page_refs *page_ref; /* scan result */
    struct engine_ops *ops;
    struct task *tasks;
    uint64_t page_cnt;          /* number of pages */
    struct engine *next;
};

struct engine_ops {
    int (*fill_eng_params)(GKeyFile *config, struct engine *eng);
    void (*clear_eng_params)(struct engine *eng);
    int (*fill_task_params)(GKeyFile *config, struct task *task);
    void (*clear_task_params)(struct task *tk);
    int (*start_task)(struct engine *eng, struct task *tk);
    void (*stop_task)(struct engine *eng, struct task *tk);
    int (*alloc_pid_params)(struct engine *eng, struct task_pid **tk_pid);
    void (*free_pid_params)(struct engine *eng, struct task_pid **tk_pid);
    int (*eng_mgt_func)(struct engine *eng, struct task *tk, char *cmd, int fd);
};

struct engine *etmemd_engine_add(GKeyFile *config);
void etmemd_engine_remove(struct engine *eng);

#endif
