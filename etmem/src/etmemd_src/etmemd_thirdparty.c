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
 * Author: shikemeng
 * Create: 2021-04-30
 * Description: Etmemd thirdparty API.
 ******************************************************************************/

#include <stdio.h>
#include <dlfcn.h>

#include "etmemd_engine.h"
#include "etmemd_file.h"
#include "etmemd_log.h"
#include "etmemd_common.h"
#include "etmemd_thirdparty.h"
#include "securec.h"

struct thirdparty_params {
    char *eng_name;
    char *libname;
    char *ops_name;
};

static int fill_eng_name(void *obj, void *val)
{
    char *eng_name = (char *)val;
    struct thirdparty_params *params = (struct thirdparty_params *)obj;

    params->eng_name = eng_name;
    return 0;
}

static int fill_libname(void *obj, void *val)
{
    char *libname = (char *)val;
    struct thirdparty_params *params = (struct thirdparty_params *)obj;

    params->libname = libname;
    return 0;
}

static int fill_ops_name(void *obj, void *val)
{
    char *ops_name = (char *)val;
    struct thirdparty_params *params = (struct thirdparty_params *)obj;

    params->ops_name = ops_name;
    return 0;
}

static struct config_item g_thirdparty_configs[] = {
    {"eng_name", STR_VAL, fill_eng_name, false},
    {"libname", STR_VAL, fill_libname, false},
    {"ops_name", STR_VAL, fill_ops_name, false},
};

static void clear_thirdparty_params(struct thirdparty_params *params)
{
    if (params->eng_name != NULL) {
        free(params->eng_name);
        params->eng_name = NULL;
    }
    if (params->libname != NULL) {
        free(params->libname);
        params->libname = NULL;
    }
    if (params->ops_name != NULL) {
        free(params->ops_name);
        params->ops_name = NULL;
    }
}

static int set_engine_ops(struct engine *eng, struct thirdparty_params *params)
{
    void *handler = NULL;
    struct engine_ops *ops = NULL;
    char *err = NULL;
    char resolve_path[PATH_MAX] = {0};

    if (realpath(params->libname, resolve_path) == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "file of thirdparty libname %s is not a real path(%s)\n",
                   params->libname, strerror(errno));
        return -1;
    }

    if (file_permission_check(resolve_path, S_IRWXU) != 0 &&
        file_permission_check(resolve_path, S_IRUSR | S_IXUSR) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "file : %s permissions do not meet requirements."
                                   "Only support 500 or 700\n", resolve_path);
        return -1;
    }

    handler = dlopen(resolve_path, RTLD_NOW | RTLD_LOCAL);
    err = dlerror();
    if (err != NULL && handler == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "load library %s fail with error: %s\n", resolve_path, err);
        return -1;
    }

    ops = dlsym(handler, params->ops_name);
    err = dlerror();
    if (err != NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "load engine_ops symbol %s fail with error: %s\n", params->ops_name, err);
        dlclose(handler);
        return -1;
    }

    eng->ops = ops;
    eng->handler = handler;
    return 0;
}

static void clear_engine_ops(struct engine *eng)
{
    dlclose(eng->handler);
    eng->handler = NULL;
    eng->ops = NULL;
}

static void set_engine_name(struct engine *eng, struct thirdparty_params *params)
{
    eng->name = params->eng_name;
    /* avoid that eng_name will be freed in clear_thirdparty_params */
    params->eng_name = NULL;
}

static void clear_engine_name(struct engine *eng)
{
    free(eng->name);
    eng->name = NULL;
}

static int set_engine(struct engine *eng, struct thirdparty_params *params)
{
    if (set_engine_ops(eng, params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set engine ops fail\n");
        return -1;
    }
    set_engine_name(eng, params);
    eng->engine_type = THIRDPARTY_ENGINE;

    return 0;
}

static void clear_engine(struct engine *eng)
{
    clear_engine_name(eng);
    clear_engine_ops(eng);
}

int fill_engine_type_thirdparty(struct engine *eng, GKeyFile *config)
{
    struct thirdparty_params params;
    int ret = -1;

    if (memset_s(&params, sizeof(struct thirdparty_params), 0, sizeof(struct thirdparty_params)) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "memset_s for thirdparty_params fail\n");
        return -1;
    }

    if (parse_file_config(config, ENG_GROUP, g_thirdparty_configs,
                ARRAY_SIZE(g_thirdparty_configs), &params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "parse thirdparty_params fail\n");
        goto clear_params;
    }

    if (set_engine(eng, &params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set engine fail\n");
        goto clear_params;
    }

    ret = 0;

clear_params:
    clear_thirdparty_params(&params);
    return ret;
}

void clear_engine_type_thirdparty(struct engine *eng)
{
    clear_engine(eng);
}
