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
 * Create: 2021-4-13
 * Description: engine operation for etmem client
 ******************************************************************************/

#include <getopt.h>
#include <stdio.h>
#include "etmem_engine.h"
#include "securec.h"
#include "etmem.h"
#include "etmem_common.h"
#include "etmem_rpc.h"

static void engine_help(void)
{
    printf("\nUsage:\n"
            "    etmem engine eng_cmd [options]\n"
            "    etmem engine help\n"
            "\nOptions:\n"
            "    -n|--proj_name <proj_name> project engine belongs to\n"
            "    -s|--socket <socket_name> socket name to connect\n"
            "    -e|--engine <engine_name> engine to execute cmd\n"
            "    -t|--task_name <task_name> task for cmd\n"
            "\nNotes:\n"
            "    1. project name must be given.\n"
            "    2. socket name must be given.\n"
            "    3. engine name must be given.\n"
            "    4. engine cmd must be given.\n"
            "    5. eng_cmd is supported by engine own.\n"
            "    6. cslide engine eng_cmd: showtaskpages, showhostpages.\n");
}

static void engine_parse_cmd(struct etmem_conf *conf, struct mem_proj *proj)
{
    proj->eng_cmd = conf->argv[0];
    proj->cmd = ETMEM_CMD_ENGINE;
    return;
}

static int engine_parse_args(struct etmem_conf *conf, struct mem_proj *proj)
{
    int opt;
    int params_cnt = 0;
    int ret;
    struct option opts[] = {
        {"proj_name", required_argument, NULL,'n'},
        {"socket", required_argument, NULL,'s'},
        {"engine", required_argument, NULL,'e'},
        {"task_name", required_argument, NULL,'t'},
        {NULL, 0, NULL,0},
    };

    while ((opt = getopt_long(conf->argc, conf->argv, "n:s:e:t:", opts, NULL)) != -1) {
        switch (opt) {
            case 'n':
                proj->proj_name = optarg;
                break;

            case 's':
                proj->sock_name = optarg;
                break;

            case 'e':
                proj->eng_name = optarg;
                break;

            case 't':
                proj->task_name = optarg;
                break;

            case '?':
                /* fallthrough */
            default:
                printf("invalid options: %s\n", conf->argv[optind]);
                return -EINVAL;
        }
        params_cnt++;

    }
    ret = etmem_parse_check_result(params_cnt, conf->argc);
    return ret;
}

static int engine_check_params(struct mem_proj *proj)
{
    if (proj->sock_name == NULL || strlen(proj->sock_name) == 0) {
        printf("socket name to connect must be given, please check.\n");
        return -1;
    }

    if (proj->proj_name == NULL || strlen(proj->proj_name) == 0) {
        printf("project name must be given, please check.\n");
        return -1;
    }

    if (proj->eng_cmd == NULL || strlen(proj->eng_cmd) == 0) {
        printf("engine command must be given, please check.\n");
        return -1;
    }

    if (proj->eng_name == NULL || strlen(proj->eng_name) == 0) {
        printf("engine name must be given, please check.\n");
        return -1;
    }

    return 0;
}

static int engine_do_cmd(struct etmem_conf *conf)
{
    struct mem_proj proj;
    int ret;

    ret = memset_s(&proj, sizeof(struct mem_proj), 0, sizeof(struct mem_proj));
    if (ret != EOK) {
        printf("memset_s for mem_proj fail\n");
        return ret;
    }

    engine_parse_cmd(conf, &proj);

    ret = engine_parse_args(conf, &proj);
    if (ret != 0) {
        printf("engine_parse_args fail\n");
        return -1;
    }

    ret = engine_check_params(&proj);
    if (ret != 0) {
        printf("engine_check_params fail\n");
        return -1;
    }

    ret = etmem_rpc_client(&proj);
    return ret;
}

static struct etmem_obj g_etmem_engine = {
    .name = "engine",
    .help = engine_help,
    .do_cmd = engine_do_cmd,
};

void engine_init(void)
{
    etmem_register_obj(&g_etmem_engine);
}

void engine_exit(void)
{
    etmem_unregister_obj(&g_etmem_engine);
}
