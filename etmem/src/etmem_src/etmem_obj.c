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
 * Create: 2021-4-14
 * Description: etmem obj command API.
 ******************************************************************************/

#include <getopt.h>
#include "securec.h"
#include "etmem.h"
#include "etmem_common.h"
#include "etmem_rpc.h"
#include "etmem_obj.h"

static void obj_help(void)
{
    printf("\nUsage:\n"
           "    etmem obj add [options]\n"
           "    etmem obj del [options]\n"
           "    etmem obj help [options]\n"
           "\nOptions:\n"
           "    -f|--file <conf_file>       Add configuration file\n"
           "    -s|--socket <socket_name>   Socket name to connect\n"
           "\nNotes:\n"
           "    1. Configuration file must be given.\n"
           "    2. Socket name must be given.\n");
}

struct obj_cmd_item {
    char *cmd_name;
    enum etmem_cmd_e cmd;
};

static struct obj_cmd_item g_obj_cmd_items[] = {
    {"add", ETMEM_CMD_ADD},
    {"del", ETMEM_CMD_DEL},
};

static int obj_parse_cmd(struct etmem_conf *conf, struct mem_proj *proj)
{
    unsigned i;
    char *cmd = NULL;

    cmd = conf->argv[0];
    for (i = 0; i < ARRAY_SIZE(g_obj_cmd_items); i++) {
        if (strcmp(cmd, g_obj_cmd_items[i].cmd_name) == 0) {
            proj->cmd = g_obj_cmd_items[i].cmd;
            return 0;
        }
    }

    printf("obj cmd %s is not supported.\n", cmd);
    return -1;
}

static int obj_parse_args(struct etmem_conf *conf, struct mem_proj *proj)
{
    int opt, ret;
    int params_cnt = 0;
    struct option opts[] = {
        {"file", required_argument, NULL, 'f'},
        {"socket", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0},
    };

    while ((opt = getopt_long(conf->argc, conf->argv, "f:s:", opts, NULL)) != -1) {
        switch (opt) {
            case 'f':
                proj->file_name = optarg;
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

static int obj_check_params(const struct mem_proj *proj)
{
    if (proj->sock_name == NULL || strlen(proj->sock_name) == 0) {
        printf("socket name to connect must all be given, please check.\n");
        return -EINVAL;
    }

    if (proj->file_name == NULL || strlen(proj->file_name) == 0) {
        printf("file name must be given in add command.\n");
        return -EINVAL;
    }

    return 0;
}

static int obj_do_cmd(struct etmem_conf *conf)
{
    struct mem_proj proj;
    int ret;

    ret = memset_s(&proj, sizeof(struct mem_proj), 0, sizeof(struct mem_proj));
    if (ret != EOK) {
        printf("memset_s for mem_proj failed\n");
        return ret;
    }

    ret = obj_parse_cmd(conf, &proj);
    if (ret != 0) {
        printf("obj_parse_cmd fail\n");
        return ret;
    }

    ret = obj_parse_args(conf, &proj);
    if (ret != 0) {
        printf("obj_parse_args fail\n");
        return ret;
    }

    ret = obj_check_params(&proj);
    if (ret != 0) {
        printf("obj_check_params fail\n");
        return ret;
    }

    ret = etmem_rpc_client(&proj);

    return ret;
}

static struct etmem_obj g_etmem_obj = {
    .name = "obj",
    .help = obj_help,
    .do_cmd = obj_do_cmd,
};

void obj_init(void)
{
    etmem_register_obj(&g_etmem_obj);
}

void obj_exit(void)
{
    etmem_unregister_obj(&g_etmem_obj);
}
