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
 * Description: File operation API.
 ******************************************************************************/

#include "etmemd_log.h"
#include "etmemd_file.h"

static int parse_item(GKeyFile *config, char *group_name, struct config_item *item, void *obj)
{
    GError *error = NULL;
    void *val;

    if (!g_key_file_has_key(config, group_name, item->key, NULL)) {
        if (item->option) {
            return 0;
        }
        etmemd_log(ETMEMD_LOG_ERR, "key %s not set for group %s\n", item->key, group_name);
        return -1;
    }

    switch (item->type) {
        case INT_VAL:
            val = (void *)(long long)g_key_file_get_int64(config, group_name, item->key, &error);
            break;
        case STR_VAL:
            val = (void *)g_key_file_get_string(config, group_name, item->key, &error);
            if (val == NULL) {
                etmemd_log(ETMEMD_LOG_ERR, "section %s of group [%s] should not be empty\n", item->key, group_name);
                return -1;
            }

            if (strlen(val) == 0) {
                free(val);
                etmemd_log(ETMEMD_LOG_ERR, "section %s of group [%s] should not be empty\n", item->key, group_name);
                return -1;
            }
            break;
        default:
            etmemd_log(ETMEMD_LOG_ERR, "config item type %d not support\n", item->type);
            return -1;
    }

    if (error != NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get value of key %s fail\n", item->key);
        return -1;
    }

    return item->fill(obj, val);
}

int parse_file_config(GKeyFile *config, char *group_name, struct config_item *items, unsigned n, void *obj)
{
    unsigned i;

    for (i = 0; i < n; i++) {
        if (parse_item(config, group_name, &items[i], obj) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "parse config key %s fail\n", items[i].key);
            return -1;
        }
    }

    return 0;
}
