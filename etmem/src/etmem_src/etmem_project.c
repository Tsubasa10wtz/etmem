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
 * Description: Etmem project command API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "securec.h"
#include "etmem.h"
#include "etmem_project.h"
#include "etmem_rpc.h"
#include "etmem_common.h"

static void project_help(void)
{
    fprintf(stdout,
            "\nUsage:\n"
            "    etmem project start [options]\n"
            "    etmem project stop [options]\n"
            "    etmem project show [options]\n"
            "    etmem project help\n"
            "\nOptions:\n"
            "    -n|--name <proj_name>     Add project name\n"
            "    -s|--socket <socket_name> Socket name to connect\n"
            "\nNotes:\n"
            "    1. Project name and socket name must be given when execute add or del option.\n"
            "    2. Socket name must be given when execute show option.\n");
}

struct project_cmd_item {
    char *cmd_name;
    enum etmem_cmd_e cmd;
};

static struct project_cmd_item g_project_cmd_items[] = {
    {"show", ETMEM_CMD_SHOW},
    {"start", ETMEM_CMD_START},
    {"stop", ETMEM_CMD_STOP},
};

static int project_parse_cmd(struct etmem_conf *conf, struct mem_proj *proj)
{
    unsigned i;
    char *cmd = NULL;

    cmd = conf->argv[0];
    for (i = 0; i < ARRAY_SIZE(g_project_cmd_items); i++) {
        if (strcmp(cmd, g_project_cmd_items[i].cmd_name) == 0) {
            proj->cmd = g_project_cmd_items[i].cmd;
            return 0;
        }
    }

    printf("project cmd %s is not supported\n", cmd);
    return -1;
}

static int project_parse_args(const struct etmem_conf *conf, struct mem_proj *proj)
{
    int opt;
    int ret;
    int params_cnt = 0;
    struct option opts[] = {
        {"name", required_argument, NULL, 'n'},
        {"socket", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0},
    };

    while ((opt = getopt_long(conf->argc, conf->argv, "n:s:",
                              opts, NULL)) != -1) {
        switch (opt) {
            case 'n':
                proj->proj_name = optarg;
                break;
            case 's':
                proj->sock_name = optarg;
                break;
            case '?':
                /* fallthrough */
            default:
                printf("invalid option: %s\n", conf->argv[optind]);
                return -EINVAL;
        }
        params_cnt++;
    }

    ret = etmem_parse_check_result(params_cnt, conf->argc);

    return ret;
}

static int project_check_params(const struct mem_proj *proj)
{
    if (proj->sock_name == NULL || strlen(proj->sock_name) == 0) {
        printf("socket name to connect must all be given, please check.\n");
        return -EINVAL;
    }

    if (proj->cmd == ETMEM_CMD_SHOW) {
        return 0;
    }

    if (proj->cmd == ETMEM_CMD_START || proj->cmd == ETMEM_CMD_STOP) {
        if (proj->proj_name == NULL || strlen(proj->proj_name) == 0) {
            printf("project name must be given, please check\n");
            return -EINVAL;
        }
    }

    return 0;
}

static int project_do_cmd(struct etmem_conf *conf)
{
    struct mem_proj proj;
    int ret;

    ret = memset_s(&proj, sizeof(struct mem_proj), 0, sizeof(struct mem_proj));
    if (ret != EOK) {
        printf("memset_s for mem_proj failed.\n");
        return ret;
    }

    ret = project_parse_cmd(conf, &proj);
    if (ret != 0) {
        return ret;
    }

    ret = project_parse_args(conf, &proj);
    if (ret != 0) {
        return ret;
    }

    ret = project_check_params(&proj);
    if (ret != 0) {
        return ret;
    }

    ret = etmem_rpc_client(&proj);

    return ret;
}

static struct etmem_obj g_etmem_project = {
    .name = "project",
    .help = project_help,
    .do_cmd = project_do_cmd,
};

void project_init(void)
{
    etmem_register_obj(&g_etmem_project);
}

void project_exit(void)
{
    etmem_unregister_obj(&g_etmem_project);
}
