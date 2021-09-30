/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * etmem/memRouter licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: zhangxuzhou
 * Create: 2020-09-08
 * Description: init policy
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <numaif.h>
#include <numa.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <json-c/json.h>
#include <json-c/json_util.h>
#include <json-c/json_object.h>

#include "memdcd_log.h"
#include "memdcd_policy_threshold.h"
#include "memdcd_policy.h"

#define FULL_PERMISSION 0777

static struct {
    char *name;
    enum mem_policy_type type;
    struct memdcd_policy_opt * (*get_opt)(void);
} mem_policy_opts[] = {
    {"mig_policy_threshold", POL_TYPE_THRESHOLD, get_threshold_policy},
    {NULL, POL_TYPE_MAX, NULL},
};

struct mem_policy g_policy;

static int check_policy_file_permission(const char *path)
{
    char error_str[ERROR_STR_MAX_LEN] = {0};
    struct stat permission_buffer;

    if (stat(path, &permission_buffer) != 0) {
        memdcd_log(_LOG_ERROR, "Get file stat failed. err: %s",
            strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        return -1;
    }

    if (permission_buffer.st_uid != geteuid()) {
        memdcd_log(_LOG_ERROR, "Owner of config file is not same with user of this process.");
        return -EACCES;
    }

    if ((permission_buffer.st_mode & FULL_PERMISSION) != (S_IRUSR | S_IWUSR)) {
        memdcd_log(_LOG_ERROR, "Access permission of config file is not 600.");
        return -EACCES;
    }

    return 0;
}

static int parse_policy_type(const char *path)
{
    int i;
    const char *str = NULL;

    json_object *root = json_object_from_file(path);
    if (root == NULL)
        return -1;

    json_object *type = json_object_object_get(root, "type");
    if (type == NULL)
        return -1;

    str = json_object_get_string(type);
    if (str == NULL)
        return -1;

    for (i = 0; mem_policy_opts[i].name != NULL; i++) {
        if (strcmp(str, mem_policy_opts[i].name) == 0) {
            return i;
        }
    }

    return -1;
}

int init_mem_policy(char *path)
{
    int ret;
    int type;
    struct memdcd_policy_opt *opt = NULL;
    char file_path[PATH_MAX] = {0};

    if (!realpath(path, file_path)) {
        memdcd_log(_LOG_ERROR, "Config file not exist.");
        return -EEXIST;
    }
    ret = check_policy_file_permission(file_path);
    if (ret != 0) {
        return ret;
    }

    type = parse_policy_type(file_path);
    if (type < 0)
        return -EINVAL;

    memdcd_log(_LOG_DEBUG, "%s: type: %d.", __func__, g_policy.type);

    opt = mem_policy_opts[type].get_opt();

    ret = opt->init(&g_policy, file_path);
    if (ret != 0)
        return ret;

    return 0;
}

struct mem_policy *get_policy(void)
{
    return &g_policy;
}
