/* *****************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: yangxin
 * Create: 2022-09-26
 * Description: Cunit test for memdcd_daemon
 * **************************************************************************** */

#include <stdlib.h>
#include <errno.h>
#include <CUnit/Automated.h>
#include "memdcd_daemon.h"

static void test_memdcd_daemon(void)
{
    CU_ASSERT_PTR_NULL(memdcd_daemon_start(NULL));
    char *sock_path_108 =
        "JINIENFIHNIENIFHNIEJHFINMEINFOINOIVNWOIENOFNOINESOIDFNOIQNFIOQWNOINQIOFNOQWNFOINDJFIJEIJFIJEOIFJOIWJEFOIJWOJ";
    char *sock_path_108_more = "JINIENfasdfefFIHNIENIFHNIEJHFINMEINFOINOIVNWOIENOFNOINESOIDFNOIQNFIOQWNOINQIOFNOQWNFOIN"
        "DJFIJEIJFIJEOIFJOIWJEFOIJWOJ";
    CU_ASSERT_PTR_NOT_NULL(memdcd_daemon_start(sock_path_108));
    CU_ASSERT_PTR_NULL(memdcd_daemon_start(sock_path_108_more))
}

int add_tests(void)
{
    /* add test case for memdcd_daemon */
    CU_pSuite suite_memdcd_daemon = CU_add_suite("memdcd_daemon", NULL, NULL);
    if (suite_memdcd_daemon == NULL) {
        return -1;
    }

    if (CU_ADD_TEST(suite_memdcd_daemon, test_memdcd_daemon) == NULL) {
        return -1;
    }

    CU_set_output_filename("memdcd");
    return 0;
}
