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
 * Description: Memigd engine API.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "etmemd_engine.h"
#include "etmemd_slide.h"
#include "etmemd_log.h"

const char *etmemd_get_eng_name(enum eng_type type)
{
    if (type == SLIDE_ENGINE) {
        return "slide";
    }

    return "";
}

struct engine_item g_eng_items[ENGINE_TYPE_CNT] = {
    {SLIDE_ENGINE, fill_engine_type_slide},
};

int fill_engine_type(struct engine *eng, const char *val)
{
    int ret = -1;
    int i;

    for (i = 0; i < ENGINE_TYPE_CNT; i++) {
        if (strcmp(val, etmemd_get_eng_name(g_eng_items[i].eng_type)) == 0) {
            ret = g_eng_items[i].fill_eng_func(eng);
            break;
        }
    }

    if (ret != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid engine type %s\n", val);
        return -1;
    }
    return 0;
}


