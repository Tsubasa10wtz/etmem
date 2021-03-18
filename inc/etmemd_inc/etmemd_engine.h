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

#include "etmemd.h"
#include "etmemd_task.h"

enum eng_type {
    SLIDE_ENGINE = 0,
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
    void *params;               /* point to engine parameter struct */
    void *task;                 /* point to task this engine belongs to */
    struct page_refs *page_ref; /* scan result */
    uint64_t page_cnt;          /* number of pages */
    struct adapter *adp;        /* configured migration strategy */

    /* parse parameter configuration */
    int (*parse_param_conf)(struct engine *eng, FILE *file);

    /* migrate policy function */
    struct memory_grade *(*mig_policy_func)(struct page_refs **page_refs, void *params);

    /* alloc tkpid params space based on different policies. */
    int (*alloc_params)(struct task_pid **tk_pid);
};

/*
 * adapter struct
 * */
struct adapter {
    /* scan function */
    struct page_refs *(*do_scan)(const struct task_pid *tpid, const struct task *tk);

    /* migrate function */
    int (*do_migrate)(unsigned int pid, const struct memory_grade *memory_grade);
};

struct engine_item {
    enum eng_type eng_type;
    int (*fill_eng_func)(struct engine *eng);
};

struct engine_private_item {
    char *priv_sec_name;
    int (*fill_eng_private_func)(const struct engine *eng, const char *val);
};

const char *etmemd_get_eng_name(enum eng_type type);

int fill_engine_type(struct engine *eng, const char *val);
#endif
