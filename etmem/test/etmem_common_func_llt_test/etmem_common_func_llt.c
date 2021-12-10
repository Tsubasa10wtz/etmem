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
 * Author: liubo
 * Create: 2021-12-07
 * Description: This is a source file of the unit test for common function in etmem.
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "etmemd_common.h"
#include "etmemd_engine.h"
#include "etmemd_rpc.h"

static void clean_flags(bool *ishelp)
{
    *ishelp = false;
    optarg = NULL;
    optind = 0;
}

static void test_parse_cmdline_error(void)
{
    bool is_help = false;
    char *cmd[] = {"./etmemd"};
    char *cmd_error_para[] = {"./etmemd", "-d"};
    char *cmd_error_help[] = {"./etmemd", "help"};
    char *cmd_err_help[] = {"./etmemd", "---help"};
    char *cmd_error_log[] = {"./etmemd", "-l"};
    char *cmd_lack_sock[] = {"./etmemd", "-l", "0"};
    char *cmd_sock_lack_para[] = {"./etmemd", "-l", "0", "-s"};
    char *cmd_sock_err[] = {"./etmemd", "-l", "0", "1", "-s"};
    char *cmd_mul_log[] = {"./etmemd", "-l", "0", "-l", "0"};
    char *cmd_mul_sock[] = {"./etmemd", "-s", "sock0", "-s", "sock1"};
    char *cmd_para_too_long[] = {"./etmemd", "-l", "0", "-l", "0", "-s", "sock"};
    char *cmd_para_redundant[] = {"./etmemd", "-l", "0", "-h", "-s", "sock"};
    char *cmd_lack_s[] = {"./etmemd", "-l", "0", "-h"};
    char *cmd_unwanted_para[] = {"./etmemd", "-l", "0", "-d", "file"};

    CU_ASSERT_EQUAL(etmemd_parse_cmdline(0, NULL, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd) / sizeof(cmd[0]), cmd, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_error_para) / sizeof(cmd_error_para[0]), cmd_error_para, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_error_help) / sizeof(cmd_error_help[0]), cmd_error_help, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_err_help) / sizeof(cmd_err_help[0]), cmd_err_help, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_error_log) / sizeof(cmd_error_log[0]), cmd_error_log, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_lack_sock) / sizeof(cmd_lack_sock[0]), cmd_lack_sock, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_sock_lack_para) / sizeof(cmd_sock_lack_para[0]), cmd_sock_lack_para, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_sock_err) / sizeof(cmd_sock_err[0]), cmd_sock_err, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_mul_log) / sizeof(cmd_mul_log[0]), cmd_mul_log, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_mul_sock) / sizeof(cmd_mul_sock[0]), cmd_mul_sock, &is_help), -1);
    etmemd_sock_name_free();
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_para_too_long) / sizeof(cmd_para_too_long[0]), cmd_para_too_long, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_para_redundant) / sizeof(cmd_para_redundant[0]), cmd_para_redundant, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_lack_s) / sizeof(cmd_lack_s[0]), cmd_lack_s, &is_help), -1);
    clean_flags(&is_help);
    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_unwanted_para) / sizeof(cmd_unwanted_para[0]), cmd_unwanted_para, &is_help), -1);
    clean_flags(&is_help);
}

static void test_parse_cmdline_ok(void)
{
    bool is_help = false;
    char *cmd[] = {"./etmemd", "-h"};
    char *cmd_h[] = {"./etmemd", "-help"};
    char *cmd_help[] = {"./etmemd", "--help"};
    char *cmd_ok[] = {"./etmemd", "-l", "0", "-s", "cmd_ok"};
    char *cmd_only_sock[] = {"./etmemd", "-s", "cmd_only_sock"};

    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd) / sizeof(cmd[0]), cmd, &is_help), 0);
    etmemd_sock_name_free();
    clean_flags(&is_help);

    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_ok) / sizeof(cmd_ok[0]), cmd_ok, &is_help), 0);
    clean_flags(&is_help);

    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_h) / sizeof(cmd_h[0]), cmd_h, &is_help), 0);
    clean_flags(&is_help);

    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_help) / sizeof(cmd_help[0]), cmd_help, &is_help), 0);
    clean_flags(&is_help);

    CU_ASSERT_EQUAL(etmemd_parse_cmdline(sizeof(cmd_only_sock) / sizeof(cmd_only_sock[0]), cmd_only_sock, &is_help), 0);
    etmemd_sock_name_free();
    clean_flags(&is_help);
}

static void test_get_int_value_error(void)
{
    int value;

    CU_ASSERT_EQUAL(get_int_value("a1", &value), -1);
    CU_ASSERT_EQUAL(get_int_value("-2147483649", &value), -1);
    CU_ASSERT_EQUAL(get_int_value("2147483648", &value), -1);
}

static void test_get_int_value_ok(void)
{
    int value;

    CU_ASSERT_EQUAL(get_int_value("-2147483648", &value), 0);
    CU_ASSERT_EQUAL(value, INT_MIN);
    CU_ASSERT_EQUAL(get_int_value("-2147483647", &value), 0);
    CU_ASSERT_EQUAL(value, INT_MIN + 1);
    CU_ASSERT_EQUAL(get_int_value("0", &value), 0);
    CU_ASSERT_EQUAL(value, 0);
    CU_ASSERT_EQUAL(get_int_value("2147483646", &value), 0);
    CU_ASSERT_EQUAL(value, INT_MAX - 1);
    CU_ASSERT_EQUAL(get_int_value("2147483647", &value), 0);
    CU_ASSERT_EQUAL(value, INT_MAX);
}

static void test_get_uint_value_error(void)
{
    unsigned int value;

    CU_ASSERT_EQUAL(get_unsigned_int_value("a1", &value), -1);
    CU_ASSERT_EQUAL(get_unsigned_int_value("-1", &value), -1);
    CU_ASSERT_EQUAL(get_unsigned_int_value("4294967296", &value), -1);
}

static void test_get_uint_value_ok(void)
{
    unsigned int value;

    CU_ASSERT_EQUAL(get_unsigned_int_value("0", &value), 0);
    CU_ASSERT_EQUAL(value, 0);
    CU_ASSERT_EQUAL(get_unsigned_int_value("1", &value), 0);
    CU_ASSERT_EQUAL(value, 1);
    CU_ASSERT_EQUAL(get_unsigned_int_value("4294967294", &value), 0);
    CU_ASSERT_EQUAL(value, UINT_MAX - 1);
    CU_ASSERT_EQUAL(get_unsigned_int_value("4294967295", &value), 0);
    CU_ASSERT_EQUAL(value, UINT_MAX);
}

static void test_get_proc_file_error(void)
{
    CU_ASSERT_PTR_NULL(etmemd_get_proc_file("0", "/maps", 0, "r"));
    CU_ASSERT_PTR_NULL(etmemd_get_proc_file("1", "maps", 0, "r"));
    CU_ASSERT_PTR_NULL(etmemd_get_proc_file("1", "/map", 0, "r"));
}

static void test_get_proc_file_ok(void)
{
    FILE *fp = NULL;

    fp = etmemd_get_proc_file("1", "/maps", 0, "r");

    CU_ASSERT_EQUAL(fclose(fp), 0);
}

static void test_get_key_value_error(void)
{
    char key[KEY_VALUE_MAX_LEN] = {};
    char value[KEY_VALUE_MAX_LEN] = {};

    CU_ASSERT_EQUAL(get_keyword_and_value("", key, value), -1);
    CU_ASSERT_EQUAL(get_keyword_and_value("abcd", key, value), -1);
    CU_ASSERT_EQUAL(get_keyword_and_value(":1", key, value), -1);
    CU_ASSERT_EQUAL(get_keyword_and_value("a:", key, value), -1);
    CU_ASSERT_EQUAL(get_keyword_and_value("#a:1", key, value), -1);
    CU_ASSERT_EQUAL(get_keyword_and_value("a.b:1", key, value), -1);
    CU_ASSERT_EQUAL(get_keyword_and_value("loop:2*4", key, value), -1);
}

static void test_get_key_value_ok(void)
{
    char key[KEY_VALUE_MAX_LEN] = {};
    char value[KEY_VALUE_MAX_LEN] = {};

    CU_ASSERT_EQUAL(get_keyword_and_value("loop:30", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "loop");
    CU_ASSERT_STRING_EQUAL(value, "30");
    CU_ASSERT_EQUAL(get_keyword_and_value("loop 30", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "loop");
    CU_ASSERT_STRING_EQUAL(value, "30");
    CU_ASSERT_EQUAL(get_keyword_and_value("loop\t30", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "loop");
    CU_ASSERT_STRING_EQUAL(value, "30");
    CU_ASSERT_EQUAL(get_keyword_and_value("loop :30", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "loop");
    CU_ASSERT_STRING_EQUAL(value, "30");
    CU_ASSERT_EQUAL(get_keyword_and_value("loop\t:30", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "loop");
    CU_ASSERT_STRING_EQUAL(value, "30");
    CU_ASSERT_EQUAL(get_keyword_and_value("loop\t 30", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "loop");
    CU_ASSERT_STRING_EQUAL(value, "30");
    CU_ASSERT_EQUAL(get_keyword_and_value("a_b:1", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "a_b");
    CU_ASSERT_STRING_EQUAL(value, "1");
    CU_ASSERT_EQUAL(get_keyword_and_value("%P:1", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "%P");
    CU_ASSERT_STRING_EQUAL(value, "1");
    CU_ASSERT_EQUAL(get_keyword_and_value("%P:0.2", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "%P");
    CU_ASSERT_STRING_EQUAL(value, "0.2");
    CU_ASSERT_EQUAL(get_keyword_and_value("%P:20%", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "%P");
    CU_ASSERT_STRING_EQUAL(value, "20%");
    CU_ASSERT_EQUAL(get_keyword_and_value("output:/var/run", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "output");
    CU_ASSERT_STRING_EQUAL(value, "/var/run");
    CU_ASSERT_EQUAL(get_keyword_and_value("engine:dynamic_fb", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "engine");
    CU_ASSERT_STRING_EQUAL(value, "dynamic_fb");
    CU_ASSERT_EQUAL(get_keyword_and_value("name:qemu-kvm", key, value), 0);
    CU_ASSERT_STRING_EQUAL(key, "name");
    CU_ASSERT_STRING_EQUAL(value, "qemu-kvm");
}

typedef enum {
    CUNIT_SCREEN = 0,
    CUNIT_XMLFILE,
    CUNIT_CONSOLE
} cu_run_mode;

void test_etmem_systemd_service_0001(void)
{
    CU_ASSERT_EQUAL(system("../etmem_common_func_llt_test/test_systemd_service.sh"), 0);
}

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

    suite = CU_add_suite("etmem_common_func", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_get_int_value_error) == NULL ||
        CU_ADD_TEST(suite, test_get_int_value_ok) == NULL ||
        CU_ADD_TEST(suite, test_get_uint_value_error) == NULL ||
        CU_ADD_TEST(suite, test_get_uint_value_ok) == NULL ||
        CU_ADD_TEST(suite, test_get_key_value_error) == NULL ||
        CU_ADD_TEST(suite, test_get_key_value_ok) == NULL ||
        CU_ADD_TEST(suite, test_parse_cmdline_error) == NULL ||
        CU_ADD_TEST(suite, test_parse_cmdline_ok) == NULL ||
        CU_ADD_TEST(suite, test_get_proc_file_error) == NULL ||
        CU_ADD_TEST(suite, test_get_proc_file_ok) == NULL ||
        CU_ADD_TEST(suite, test_etmem_systemd_service_0001) == NULL) {
            printf("CU_ADD_TEST fail. \n");
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_common.c");
            CU_automated_run_tests();
            break;
        case CUNIT_CONSOLE:
            CU_console_run_tests();
            break;
        default:
            printf("not support cunit mode, only support: 0 or 1\n");
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
