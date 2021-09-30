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
 * Description: main function of memdcd.
 ******************************************************************************/
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/capability.h>
#include <time.h>
#include <unistd.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>

#include "memdcd_policy.h"
#include "memdcd_daemon.h"
#include "memdcd_log.h"

#define DEFAULT_SOCK_PATH "@_memdcd.server"
#define REQUIRE_CAP_MAX 1
#define TIMEOUT_VALID_LEN 10

static void usage(void)
{
    printf("Usage: memdcd -p|--policy <policy file> [options]\n");
    printf("       -p --policy <file>           specify policy config file\n");
    printf("       -s --socket <socket_path>    specify socket path. default: %s\n", DEFAULT_SOCK_PATH);
    printf("       -l --log LOG_INFO|LOG_DEBUG  set log level to print. default: LOG_INFO\n");
    printf("       -t --timeout <timeout>       set timeout for collect pages by second.\n");
    printf("                                    set 0 to disable timeout. default: %d\n", DEFAULT_COLLECT_PAGE_TIMEOUT);
    printf("       -h --help                    show this help info\n");
}

static int check_permission(void)
{
    cap_t cap = NULL;
    cap_flag_value_t cap_flag_value = CAP_CLEAR;
    cap_value_t cap_val = 0;
    const char *req_cap[REQUIRE_CAP_MAX] = {
        "cap_sys_nice"
    };

    cap = cap_get_proc();
    if (cap == NULL) {
        memdcd_log(_LOG_ERROR, "Get capability error.");
        return -1;
    }
    for (int i = 0; i < REQUIRE_CAP_MAX; i++) {
        cap_from_name(req_cap[i], &cap_val);
        cap_get_flag(cap, cap_val, CAP_EFFECTIVE, &cap_flag_value);
        if (cap_flag_value != CAP_SET) {
            memdcd_log(_LOG_ERROR, "Not sufficient capacity: %s.", req_cap[i]);
            cap_free(cap);
            return -1;
        }
    }
    cap_free(cap);
    return 0;
}

int main(int argc, char *argv[])
{
    int opt;
    struct option long_options[] = {
        {"policy", required_argument, NULL, 'p'},
        {"socket", required_argument, NULL, 's'},
        {"log", required_argument, NULL, 'l'},
        {"timeout", required_argument, NULL, 't'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0},
    };
    time_t timeout;
    char *policy_file = NULL, *socket_path = NULL;
    char *endptr = NULL;

    while ((opt = getopt_long(argc, argv, "p:s:l:t:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                policy_file = optarg;
                break;
            case 's':
                socket_path = optarg;
                break;
            case 'l':
                if (init_log_level(optarg) < 0) {
                    printf("error parsing log level: %s\n", optarg);
                    return -EINVAL;
                }
                break;
            case 't':
                errno = 0;
                timeout = strtol(optarg, &endptr, TIMEOUT_VALID_LEN);
                if (errno || optarg == endptr || (endptr && *endptr != ' ' && *endptr != '\0') || timeout < 0) {
                    printf("error parsing timeout %s\n", optarg);
                    return -EINVAL;
                }
                init_collect_pages_timeout(timeout);
                break;
            case 'h':
                usage();
                return 0;
            default:
                usage();
                return -EINVAL;
        }
    }

    if (policy_file == NULL) {
        printf("policy file should be assigned\n");
        usage();
        return 0;
    }
    if (socket_path == NULL)
        socket_path = DEFAULT_SOCK_PATH;

    if (check_permission() != 0) {
        memdcd_log(_LOG_ERROR, "This program is lack of nessary privileges.");
        return -1;
    }

    if (init_mem_policy(policy_file) != 0) {
        memdcd_log(_LOG_ERROR, "Error parsing policy from policy config file %s.", policy_file);
        return -1;
    }

    memdcd_daemon_start(socket_path);

    return 0;
}
