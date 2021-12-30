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
 * Description: Etmemd engine API.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "etmemd_engine.h"
#include "etmemd_slide.h"
#include "etmemd_cslide.h"
#include "etmemd_memdcd.h"
#include "etmemd_damon.h"
#include "etmemd_thirdparty.h"
#include "etmemd_log.h"
#include "etmemd_common.h"
#include "etmemd_file.h"

struct engine_add_item {
    char *name;
    int (*fill_eng_func)(struct engine *eng, GKeyFile *config);
};

struct engine_remove_item {
    int type;
    void (*clear_eng_func)(struct engine *eng);
};

static struct engine_add_item g_engine_add_items[] = {
    {"slide", fill_engine_type_slide},
    {"cslide", fill_engine_type_cslide},
    {"memdcd", fill_engine_type_memdcd},
    {"damon", fill_engine_type_damon},
    {"thirdparty", fill_engine_type_thirdparty},
};

static struct engine_add_item *find_engine_add_item(const char *name)
{
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(g_engine_add_items); i++) {
        if (strcmp(name, g_engine_add_items[i].name) == 0) {
            return &g_engine_add_items[i];
        }
    }

    return NULL;
}

static struct engine_remove_item g_engine_remove_items[] = {
    {THIRDPARTY_ENGINE, clear_engine_type_thirdparty},
};

static struct engine_remove_item *find_engine_remove_item(int type)
{
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(g_engine_remove_items); i++) {
        if (g_engine_remove_items[i].type == type) {
            return &g_engine_remove_items[i];
        }
    }

    return NULL;
}

struct engine *etmemd_engine_add(GKeyFile *config)
{
    struct engine *eng = NULL;
    struct engine_add_item *item = NULL;
    char *name = NULL;

    if (g_key_file_has_key(config, ENG_GROUP, "name", NULL) == FALSE) {
        etmemd_log(ETMEMD_LOG_ERR, "engine name is not set\n");
        return NULL;
    }

    name = g_key_file_get_string(config, ENG_GROUP, "name", NULL);
    if (name == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get name string of engine fail\n");
        return NULL;
    }

    item = find_engine_add_item(name);
    if (item == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "engine %s not support\n", name);
        goto free_name;
    }

    eng = calloc(1, sizeof(struct engine));
    if (eng == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc memory for engine fail\n");
        goto free_name;
    }

    if (item->fill_eng_func(eng, config) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "fill engine %s fail\n", name);
        free(eng);
        eng = NULL;
        goto free_name;
    }

    if (eng->ops == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "engine %s without operations\n", name);
        free(eng);
        eng = NULL;
    }

free_name:
    free(name);
    return eng;
}

void etmemd_engine_remove(struct engine *eng)
{
    struct engine_remove_item *item = NULL;

    item = find_engine_remove_item(eng->engine_type);
    if (item != NULL) {
        item->clear_eng_func(eng);
    }

    free(eng);
}
