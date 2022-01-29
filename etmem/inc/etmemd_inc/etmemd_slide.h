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
 * Description: This is a header file of the function declaration for slide function.
 ******************************************************************************/

#ifndef ETMEMD_SLIDE_H
#define ETMEMD_SLIDE_H

#include "etmemd_pool_adapter.h"
#include "etmemd_engine.h"

struct slide_params {
    struct task_executor *executor;
    int t;          /* watermark */
    unsigned long swap_threshold;
    uint8_t dram_percent;
};

enum swap_type {
    DONT_SWAP = 0,
    DO_SWAP,
};

int fill_engine_type_slide(struct engine *eng, GKeyFile *config);

#endif
