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
 * Description: Main function of etmem.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "securec.h"
#include "etmem.h"
#include "etmem_obj.h"
#include "etmem_project.h"
#include "etmem_engine.h"

SLIST_HEAD(etmem_obj_list, etmem_obj) g_etmem_objs;

static void usage(void)
{
    fprintf(stdout,
            "\nUsage:\n"
            "    etmem OBJECT COMMAND\n"
            "    etmem help\n"
            "\nParameters:\n"
            "    OBJECT  := { project | obj | engine }\n"
            "    COMMAND := { add | del | start | stop | show | eng_cmd | help }\n");
}

static struct etmem_obj *etmem_obj_get(const char *name)
{
    struct etmem_obj *obj = NULL;

    SLIST_FOREACH(obj, &g_etmem_objs, entry) {
        if (strcmp(obj->name, name) == 0) {
            return obj;
        }
    }

    return NULL;
}

static int check_param(int argc, char *argv[], struct etmem_conf *conf,
                       struct etmem_obj **obj)
{
    conf->obj = argv[0];
    *obj = etmem_obj_get(conf->obj);
    if (*obj == NULL) {
        usage();
        return -1;
    }

    if (argc < 2) { /* 2 means OBJECT and COMMAND must both be given */
        (*obj)->help();
        return -1;
    }

    argc--;
    argv++;
    conf->argc = argc;
    conf->argv = argv;

    return 0;
}

static int parse_args(int argc, char *argv[], struct etmem_conf *conf,
                      struct etmem_obj **obj)
{
    int ret;

    ret = memset_s(conf, sizeof(struct etmem_conf), 0, sizeof(struct etmem_conf));
    if (ret != EOK) {
        printf("[%s] failed\n", __func__);
        return ret;
    }

    if (argc <= 1) { /* 1 means etmem must be given parameters */
        usage();
        return -EINVAL;
    }

    argc--;
    argv++;

    if (check_param(argc, argv, conf, obj) != 0) {
        return -EINVAL;
    }

    return 0;
}

void etmem_register_obj(struct etmem_obj *obj)
{
    SLIST_INSERT_HEAD(&g_etmem_objs, obj, entry);
}

void etmem_unregister_obj(const struct etmem_obj *obj)
{
    if (!SLIST_EMPTY(&g_etmem_objs)) {
        SLIST_REMOVE(&g_etmem_objs, obj, etmem_obj, entry);
    }
}

int main(int argc, char *argv[])
{
    struct etmem_conf conf;
    struct etmem_obj *obj = NULL;
    int err = 1;

    SLIST_INIT(&g_etmem_objs);
    project_init();
    obj_init();
    engine_init();

    if (parse_args(argc, argv, &conf, &obj) != 0) {
        if (conf.obj != NULL && strcmp(conf.obj, "help") == 0 &&
            argc == 2) { /* 2 is for param num of "etmem help" */
            err = 0;
            goto out;
        }
        err = -EINVAL;
        goto out;
    }

    if (strcmp(conf.argv[0], "help") == 0) {
        obj->help();
        err = 0;
        goto out;
    }

    err = obj->do_cmd(&conf);
    if (err != 0) {
        printf("etmem: error %d\n", err);
        obj->help();
        goto out;
    }

out:
    engine_exit();
    obj_exit();
    project_exit();
    return err;
}
