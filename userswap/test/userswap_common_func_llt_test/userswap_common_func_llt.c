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
 * Description: This is a source file of the unit test for common functions in uswap.
 ******************************************************************************/

#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <errno.h>

#include "uswap_api.h"
#include "uswap_log.h"

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#define USWAP_LOG_WRONG (-1)

static void test_uswap_set_log_level(void)
{
    CU_ASSERT_EQUAL(set_uswap_log_level(USWAP_LOG_WRONG), -EINVAL);
    CU_ASSERT_EQUAL(set_uswap_log_level(USWAP_LOG_DEBUG), 0);
    CU_ASSERT_EQUAL(set_uswap_log_level(USWAP_LOG_INFO), 0);
    CU_ASSERT_EQUAL(set_uswap_log_level(USWAP_LOG_WARN), 0);
    CU_ASSERT_EQUAL(set_uswap_log_level(USWAP_LOG_ERR), 0);
    CU_ASSERT_EQUAL(set_uswap_log_level(USWAP_LOG_INVAL), -EINVAL);
}

static void test_uswap_register_userfaultfd(void)
{
    CU_ASSERT_EQUAL(register_userfaultfd(NULL, 10), USWAP_ERROR);
    CU_ASSERT_EQUAL(register_userfaultfd((void *)2, SSIZE_MAX), USWAP_ERROR);
    CU_ASSERT_EQUAL(register_userfaultfd((void *)2, 1024), USWAP_ERROR);
}

static void test_uswap_unregister_userfaultfd(void)
{
    CU_ASSERT_EQUAL(unregister_userfaultfd((void *)2, 1024), USWAP_ERROR);
    CU_ASSERT_EQUAL(unregister_userfaultfd(NULL, 10), USWAP_ERROR);
    CU_ASSERT_EQUAL(unregister_userfaultfd((void *)2, SSIZE_MAX), USWAP_ERROR);
}

static int test_get_swapout_buf(const void *addr, size_t len, struct swap_data *data)
{
    return 0;
}

static int test_do_swapout(struct swap_data *data)
{
    return 0;
}

static int test_do_swapin(const void *addr, struct swap_data *data)
{
    return 0;
}

static int test_release_buf(struct swap_data *data)
{
    return 0;
}

static struct uswap_operations test_ops = {
    .get_swapout_buf = test_get_swapout_buf,
    .do_swapout = test_do_swapout,
    .do_swapin = test_do_swapin,
    .release_buf = test_release_buf
};

static void test_uswap_register_ops(void)
{
    CU_ASSERT_EQUAL(register_uswap(NULL, 4, &test_ops), USWAP_ERROR);
    CU_ASSERT_EQUAL(register_uswap("test", MAX_USWAP_NAME_LEN, &test_ops), USWAP_ERROR);
    CU_ASSERT_EQUAL(register_uswap("test", 4, NULL), USWAP_ERROR);

    struct uswap_operations tmp = test_ops;
    tmp.get_swapout_buf = NULL;
    CU_ASSERT_EQUAL(register_uswap("test", 4, &tmp), USWAP_ERROR);
    tmp = test_ops;
    tmp.do_swapout = NULL;
    CU_ASSERT_EQUAL(register_uswap("test", 4, &tmp), USWAP_ERROR);
    tmp = test_ops;
    tmp.do_swapin = NULL;
    CU_ASSERT_EQUAL(register_uswap("test", 4, &tmp), USWAP_ERROR);
    tmp = test_ops;
    tmp.release_buf = NULL;
    CU_ASSERT_EQUAL(register_uswap("test", 4, &tmp), USWAP_ERROR);
    CU_ASSERT_EQUAL(register_uswap("test", 4, &test_ops), USWAP_SUCCESS);
}

static void test_uswap_init(void)
{
    CU_ASSERT_EQUAL(uswap_init(MAX_SWAPIN_THREAD_NUMS + 1), USWAP_ERROR);
    CU_ASSERT_EQUAL(uswap_init(MAX_SWAPIN_THREAD_NUMS), USWAP_SUCCESS);
    CU_ASSERT_EQUAL(uswap_init(MAX_SWAPIN_THREAD_NUMS), USWAP_ERROR);
}

static void test_force_swapout(void)
{
    char *addr = malloc(65536);
    if (addr < 0) {
	printf("malloc failed\n");
	return;
    }
    CU_ASSERT_EQUAL(force_swapout(NULL, 4096), USWAP_ERROR);
    CU_ASSERT_EQUAL(force_swapout(addr, SSIZE_MAX), USWAP_ERROR);
    free(addr);
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

    suite = CU_add_suite("uswap_api", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_uswap_set_log_level) == NULL ||
        CU_ADD_TEST(suite, test_uswap_register_userfaultfd) == NULL ||
	CU_ADD_TEST(suite, test_uswap_unregister_userfaultfd) == NULL ||
	CU_ADD_TEST(suite, test_uswap_register_ops) == NULL ||
	CU_ADD_TEST(suite, test_uswap_init) == NULL ||
	CU_ADD_TEST(suite, test_force_swapout) == NULL) {
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
