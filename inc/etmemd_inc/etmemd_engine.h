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
#include "etmemd_engine_exp.h"

enum eng_type {
    SLIDE_ENGINE = 0,
    CSLIDE_ENGINE,
    DYNAMIC_FB_ENGINE,
    HISTORICAL_FB_ENGINE,
    THIRDPARTY_ENGINE,
    ENGINE_TYPE_CNT,
};

struct engine *etmemd_engine_add(GKeyFile *config);
void etmemd_engine_remove(struct engine *eng);

#endif
