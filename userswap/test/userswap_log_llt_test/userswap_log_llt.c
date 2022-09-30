/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2022. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: liuyongqiang
 * Create: 2022-09-25
 * Description: This is a source file of the unit test for log functions in uswap.
 ******************************************************************************/

#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uswap_log.h"

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#define USWAP_LOG_WRONG (-1)

static void test_basic_log_level(void)
{
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_WRONG), -EINVAL);
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_DEBUG), 0);
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_INFO), 0);
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_WARN), 0);
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_ERR), 0);
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_INVAL), -EINVAL);
}

static void test_uswap_log_print(void)
{
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_DEBUG), 0);
    uswap_log(USWAP_LOG_DEBUG, "test_uswap_log_debug_level\n");
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_INFO), 0);
    uswap_log(USWAP_LOG_INFO, "test_uswap_log_info_level\n");
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_WARN), 0);
    uswap_log(USWAP_LOG_WARN, "test_uswap_log_warn_level\n");
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_ERR), 0);
    uswap_log(USWAP_LOG_ERR, "test_uswap_log_error_level\n");
    CU_ASSERT_EQUAL(uswap_log_level_init(USWAP_LOG_INVAL), -EINVAL);
    uswap_log(USWAP_LOG_INVAL, "test_uswap_log_invalid_level\n");
}

typedef enum {
    CUNIT_SCREEN = 0,
    CUNIT_XMLFILE,
    CUNIT_CONSOLE
} cu_run_mode;

int main(int argc, const char **argv)
{
    CU_pSuite suite;
    CU_pTest pTest;
    unsigned int num_failures;
    cu_run_mode cunit_mode = CUNIT_SCREEN;
    int error_num;

    if (argc > 1) {
        cunit_mode = atoi(argv[1]);
    }

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return -CU_get_error();
    }

    suite = CU_add_suite("uswap_log", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_basic_log_level) == NULL ||
        CU_ADD_TEST(suite, test_uswap_log_print) == NULL) {
            printf("CU_ADD_TEST fail. \n");
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("uswap_log.c");
            CU_automated_run_tests();
            break;
        case CUNIT_CONSOLE:
            CU_console_run_tests();
            break;
        default:
            printf("not support cunit mode, only support: "
                   "0 for CUNIT_SCREEN, 1 for CUNIT_XMLFILE, 2 for CUNIT_CONSOLE\n");
            goto ERROR;
    }

    num_failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    return num_failures;

ERROR:
    error_num = CU_get_error();
    CU_cleanup_registry();
    return -error_num;
}
