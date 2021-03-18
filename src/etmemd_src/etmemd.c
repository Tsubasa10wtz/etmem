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
 * Description: Main function of etmemd.
 ******************************************************************************/

#include <signal.h>
#include <unistd.h>

#include "etmemd_log.h"
#include "etmemd_rpc.h"
#include "etmemd_common.h"
#include "etmemd_project.h"
#include "etmemd_scan.h"

int main(int argc, char *argv[])
{
    int ret;
    bool is_help = false;

    ret = etmemd_parse_cmdline(argc, argv, &is_help);
    if (ret != 0) {
        printf("fail to parse parameters to start etmemd\n");
        return -1;
    }
    if (is_help) {
        return 0;
    }

    if (init_g_page_size() == -1) {
        return -1;
    }

    etmemd_handle_signal();
    if (etmemd_rpc_server() != 0) {
        printf("fail to start rpc server of etmemd\n");
    }

    etmemd_stop_all_projects();
    return 0;
}
