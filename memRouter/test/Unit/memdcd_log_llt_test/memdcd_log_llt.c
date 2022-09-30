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
 * Description: Cunit test for memdcd_log
 * **************************************************************************** */

#include <stdlib.h>
#include <errno.h>
#include <CUnit/Automated.h>
#include "memdcd_log.h"
#include "alloc_memory.h"

static void test_memdcd_log(void)
{
    CU_ASSERT_EQUAL(init_log_level(NULL), _LOG_INFO);
    CU_ASSERT_EQUAL(init_log_level("LOG_DEBUG"), _LOG_DEBUG);
    CU_ASSERT_EQUAL(init_log_level("LOG_INFO"), _LOG_INFO);
    CU_ASSERT_EQUAL(init_log_level("LOG_WARN"), _LOG_WARN);
    CU_ASSERT_EQUAL(init_log_level("LOG_ERROR"), _LOG_ERROR);
    CU_ASSERT_EQUAL(set_log_level(_LOG_ERROR), _LOG_ERROR);
    CU_ASSERT_EQUAL(set_log_level(_LOG_WARN), _LOG_WARN);
    CU_ASSERT_EQUAL(set_log_level(_LOG_INFO), _LOG_INFO);
    CU_ASSERT_EQUAL(set_log_level(_LOG_DEBUG), _LOG_DEBUG);
    memdcd_log(_LOG_ERROR, "memdcd error");
    memdcd_log(_LOG_WARN, "memdcd warn");
    memdcd_log(_LOG_INFO, "memdcd info");
    memdcd_log(_LOG_DEBUG, "memdcd debug");
}

int add_tests(void)
{
    /* add test case for memdcd_log */
    CU_pSuite suite_memdcd_log = CU_add_suite("memdcd_log", NULL, NULL);
    if (suite_memdcd_log == NULL) {
        return -1;
    }

    if (CU_ADD_TEST(suite_memdcd_log, test_memdcd_log) == NULL) {
        return -1;
    }

    CU_set_output_filename("memdcd");
    return 0;
}
