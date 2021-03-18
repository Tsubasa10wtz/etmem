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
 * Description: Migration command API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "securec.h"
#include "etmem.h"
#include "etmem_task.h"
#include "etmem_common.h"
#include "etmem_rpc.h"

static void migrate_help(void)
{
    fprintf(stderr,
            "\nUsage:\n"
            "    etmem migrate start [options]\n"
            "    etmem migrate stop  [options]\n"
            "    etmem migrate help\n"
            "\nOptions:\n"
            "    -n|--name <proj_name>     Add project name\n"
            "    -s|--socket <socket_name> Socket name to connect\n"
            "\nNotes:\n"
            "    Project name and socket name must be given when execute start or stop option.\n");
}

static int migrate_parse_cmd(const struct etmem_conf *conf, struct mem_proj *proj)
{
    switch (conf->cmd) {
        case ETMEM_CMD_START:
            /* fallthrough */
        case ETMEM_CMD_STOP:
            goto EXIT;
        default:
            printf("invalid command %u of migrate\n", conf->cmd);
            return -EINVAL;
    }

EXIT:
    proj->cmd = conf->cmd;
    return 0;
}

static int migrate_parse_args(const struct etmem_conf *conf, struct mem_proj *proj)
{
    int opt;
    int params_cnt = 0;
    int ret;
    struct option opts[] = {
        {"name", required_argument, NULL, 'n'},
        {"socket", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0},
    };

    while ((opt = getopt_long(conf->argc, conf->argv, "n:s:",
                              opts, NULL)) != -1) {
        switch (opt) {
            case 'n':
                ret = parse_name_string(optarg, &proj->proj_name, PROJECT_NAME_MAX_LEN);
                if (ret != 0) {
                    printf("parse project name failed.\n");
                    return ret;
                }
                break;
            case 's':
                ret = parse_name_string(optarg, &proj->sock_name, SOCKET_NAME_MAX_LEN);
                if (ret != 0) {
                    printf("parse socket name failed.\n");
                    return ret;
                }
                break;
            case '?':
                /* fallthrough */
            default:
                ret = -EINVAL;
                printf("invalid option: %s\n", conf->argv[optind]);
                return ret;
        }
        params_cnt++;
    }

    ret = etmem_parse_check_result(params_cnt, conf->argc);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

static int migrate_check_params(const struct mem_proj *proj)
{
    if (proj->proj_name == NULL || strlen(proj->proj_name) == 0 ||
        proj->sock_name == NULL || strlen(proj->sock_name)  == 0) {
        printf("project name and socket name must all be given, please check.\n");
        return -EINVAL;
    }

    return 0;
}

static int migrate_do_cmd(const struct etmem_conf *conf)
{
    struct mem_proj proj;
    int ret;

    ret = memset_s(&proj, sizeof(struct mem_proj), 0, sizeof(struct mem_proj));
    if (ret != EOK) {
        printf("memset_s for mem_proj failed.\n");
        return ret;
    }

    ret = migrate_parse_cmd(conf, &proj);
    if (ret != 0) {
        goto EXIT;
    }

    ret = migrate_parse_args(conf, &proj);
    if (ret != 0) {
        goto EXIT;
    }

    ret = migrate_check_params(&proj);
    if (ret != 0) {
        goto EXIT;
    }

    etmem_rpc_client(&proj);

EXIT:
    free_proj_member(&proj);
    return ret;
}

static struct etmem_obj g_etmem_migrate = {
    .name = "migrate",
    .help = migrate_help,
    .do_cmd = migrate_do_cmd,
};

void migrate_init(void)
{
    etmem_register_obj(&g_etmem_migrate);
}

void migrate_exit(void)
{
    etmem_unregister_obj(&g_etmem_migrate);
}
