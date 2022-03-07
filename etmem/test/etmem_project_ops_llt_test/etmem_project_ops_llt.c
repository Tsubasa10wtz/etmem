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
 * Create: 2021-11-29
 * Description: This is a source file of the unit test for project-related commands in etmem.
 ******************************************************************************/

#include <sys/file.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <glib.h>

#include "etmemd_project.h"
#include "etmemd_file.h"
#include "etmemd_common.h"
#include "securec.h"

#include "test_common.h"

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#define PROJECT_ADD_TEST_MAX_NUM 50

static struct proj_test_param g_proj_test_param;

static void etmem_pro_add_name(void)
{
    const char *project_name_long = "project name cannot longer than 32 ppmpppppppppppppppppppppppppppppppppppppppppppppp";
    GKeyFile *config = NULL;
    struct proj_test_param param;

    init_proj_param(&param);

    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    destroy_proj_config(config);

    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_PRO_EXISTED);
    destroy_proj_config(config);

    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.proj_name = "";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.proj_name = project_name_long;
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.proj_name = DEFAULT_PROJ;
}

static void etmem_pro_add_interval(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    param.interval = "0";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.interval = "1";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.interval = "2";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.interval = "1199";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.interval = "1200";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.interval = "1201";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.interval = "abc";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);
}

static void etmem_pro_add_sysmem_threshold_error(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    param.sysmem_threshold = "101";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.sysmem_threshold = "abc";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.sysmem_threshold = "a10b";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.sysmem_threshold = "10a";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.sysmem_threshold = "-1";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);
}

static void etmem_pro_add_sysmem_threshold_ok(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    param.sysmem_threshold = "0";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.sysmem_threshold = "1";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.sysmem_threshold = "2";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.sysmem_threshold = "99";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.sysmem_threshold = "100";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);
}

static void etmem_pro_add_swapcache_mark_error(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    param.swapcache_high_wmark = "-1";
    param.swapcache_low_wmark = "-1";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.swapcache_high_wmark = "1";
    param.swapcache_low_wmark = "-1";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.swapcache_high_wmark = "-1";
    param.swapcache_low_wmark = "1";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.swapcache_high_wmark = "1";
    param.swapcache_low_wmark = "2";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.swapcache_high_wmark = "101";
    param.swapcache_low_wmark = "100";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.swapcache_high_wmark = "100";
    param.swapcache_low_wmark = "101";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.swapcache_high_wmark = "101";
    param.swapcache_low_wmark = "101";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.swapcache_high_wmark = "5";
    param.swapcache_low_wmark = NULL;
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.swapcache_high_wmark = NULL;
    param.swapcache_low_wmark = "5";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);
}

static void etmem_pro_add_swapcache_mark_ok(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    param.swapcache_high_wmark = "2";
    param.swapcache_low_wmark = "1";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.swapcache_high_wmark = "100";
    param.swapcache_low_wmark = "99";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);
}

static void etmem_pro_add_loop(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    param.loop = "0";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.loop = "1";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.loop = "2";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.loop = "119";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.loop = "120";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.loop = "121";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.loop = "abc";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);
}

static void etmem_pro_add_sleep(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    param.sleep = "0";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.sleep = "1";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.sleep = "2";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.sleep = "1199";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.sleep = "1200";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.sleep = "1201";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);

    param.sleep = "wrong sleep type";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);
}

static void etmem_pro_lack_loop(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    param.loop = NULL;
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_INVAL);
    destroy_proj_config(config);
}

void test_etmem_prj_add_error(void)
{
    etmem_pro_add_name();
    etmem_pro_add_interval();
    etmem_pro_add_loop();
    etmem_pro_add_sleep();
    etmem_pro_lack_loop();
    etmem_pro_add_sysmem_threshold_error();
    etmem_pro_add_swapcache_mark_error();
}

void test_etmem_prj_del_error(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    param.proj_name = "";
    config = construct_proj_config(&param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    param.proj_name = "noexist";
    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_PRO_NOEXIST);
    destroy_proj_config(config);
}

static void add_project_once(void)
{
    struct proj_test_param param;
    GKeyFile *config = NULL;

    etmem_pro_add_sysmem_threshold_ok();
    etmem_pro_add_swapcache_mark_ok();
    init_proj_param(&param);

    CU_ASSERT_EQUAL(etmemd_project_show(NULL, 0), OPT_SUCCESS);

    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
    destroy_proj_config(config);

    CU_ASSERT_EQUAL(etmemd_project_show(NULL, 0), OPT_SUCCESS);

    config = construct_proj_config(&param);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
    destroy_proj_config(config);

    CU_ASSERT_EQUAL(etmemd_project_show("noexist", 0), OPT_SUCCESS);
}

static int add_project_multiple(int proj_add_num)
{
    char project_name_str[PROJECT_NAME_MAX_LEN] = {0};
    unsigned int project_num;
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    for (project_num = 0; project_num < proj_add_num; project_num++) {
        if (snprintf_s(project_name_str, PROJECT_NAME_MAX_LEN, PROJECT_NAME_MAX_LEN - 1, "project_add_del_test_num%d",
            project_num) <= 0) {
            printf("get project_name wrong.\n");
            return -1;
        }
        param.proj_name = project_name_str;
        config = construct_proj_config(&param);
        CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
        destroy_proj_config(config);
    }

    CU_ASSERT_EQUAL(etmemd_project_show(NULL, 0), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_show("project_add_del_test_num5", 0), OPT_SUCCESS);

    for (project_num = 0; project_num < proj_add_num; project_num++) {
        if (snprintf_s(project_name_str, PROJECT_NAME_MAX_LEN, PROJECT_NAME_MAX_LEN - 1, "project_add_del_test_num%d",
            project_num) <= 0) {
            printf("get project_name wrong.\n");
            return -1;
        }
        param.proj_name = project_name_str;
        config = construct_proj_config(&param);
        CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
        destroy_proj_config(config);
    }

    return 0;
}

void test_etmem_project_add_ok(void)
{
    add_project_once();
    CU_ASSERT_EQUAL(add_project_multiple(PROJECT_ADD_TEST_MAX_NUM), 0);
}

void test_etmem_mig_start_error(void)
{
    struct proj_test_param param;

    init_proj_param(&param);

    CU_ASSERT_EQUAL(etmemd_migrate_start(NULL), OPT_INVAL);
    CU_ASSERT_EQUAL(etmemd_migrate_start(""), OPT_PRO_NOEXIST);
    CU_ASSERT_EQUAL(etmemd_migrate_start("etmem"), OPT_PRO_NOEXIST);
    CU_ASSERT_EQUAL(etmemd_migrate_start("me^$%*mig"), OPT_PRO_NOEXIST);
 
    param.proj_name = "add_for_migrate_test";
    do_add_proj_test(&param);

    CU_ASSERT_EQUAL(etmemd_migrate_start("add_for_migrate_test"), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start("add_for_migrate_test"), OPT_PRO_STARTED);

    etmemd_stop_all_projects();
}

void test_etmem_mig_stop_error(void)
{
    struct proj_test_param param;

    init_proj_param(&param);

    CU_ASSERT_EQUAL(etmemd_migrate_stop(NULL), OPT_INVAL);
    CU_ASSERT_EQUAL(etmemd_migrate_stop(""), OPT_PRO_NOEXIST);
    CU_ASSERT_EQUAL(etmemd_migrate_stop("ETMEM"), OPT_PRO_NOEXIST);
    CU_ASSERT_EQUAL(etmemd_migrate_stop("ET^$%*MEM"), OPT_PRO_NOEXIST);

    param.proj_name = "add_for_migrate_stop_test";
    do_add_proj_test(&param);

    CU_ASSERT_EQUAL(etmemd_migrate_start("add_for_migrate_stop_test"), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_stop("add_for_migrate_stop_test"), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_stop("add_for_migrate_stop_test"), OPT_PRO_STOPPED);

    etmemd_stop_all_projects();
}

static int start_project_multiple(int proj_add_num)
{
    char project_name_str[PROJECT_NAME_MAX_LEN] = {0};
    unsigned int project_num;
    struct proj_test_param param;
    GKeyFile *config = NULL;

    init_proj_param(&param);

    for (project_num = 0; project_num < proj_add_num; project_num++) {
        if (snprintf_s(project_name_str, PROJECT_NAME_MAX_LEN, PROJECT_NAME_MAX_LEN - 1, "project_add_del_test_num%d",
            project_num) <= 0) {
            printf("get project_name wrong.\n");
            return -1;
        }
        param.proj_name = project_name_str;
        config = construct_proj_config(&param);
        CU_ASSERT_EQUAL(etmemd_project_add(config), OPT_SUCCESS);
        destroy_proj_config(config);
        CU_ASSERT_EQUAL(etmemd_migrate_start(project_name_str), OPT_SUCCESS);
    }

    for (project_num = 0; project_num < proj_add_num; project_num++) {
        if (snprintf_s(project_name_str, PROJECT_NAME_MAX_LEN, PROJECT_NAME_MAX_LEN - 1, "project_add_del_test_num%d",
            project_num) <= 0) {
            printf("get project_name wrong.\n");
            return -1;
        }
        param.proj_name = project_name_str;
        config = construct_proj_config(&param);
        CU_ASSERT_EQUAL(etmemd_migrate_stop(project_name_str), OPT_SUCCESS);
        CU_ASSERT_EQUAL(etmemd_project_remove(config), OPT_SUCCESS);
        destroy_proj_config(config);
    }

    return 0;
}

void test_etmem_mig_start_ok(void)
{
    CU_ASSERT_EQUAL(start_project_multiple(1), 0);
    CU_ASSERT_EQUAL(start_project_multiple(PROJECT_ADD_TEST_MAX_NUM), 0);
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

    suite = CU_add_suite("etmem_project_ops", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_etmem_prj_add_error) == NULL ||
        CU_ADD_TEST(suite, test_etmem_prj_del_error) == NULL ||
        CU_ADD_TEST(suite, test_etmem_project_add_ok) == NULL ||
        CU_ADD_TEST(suite, test_etmem_mig_start_error) == NULL ||
        CU_ADD_TEST(suite, test_etmem_mig_stop_error) == NULL ||
        CU_ADD_TEST(suite, test_etmem_mig_start_ok) == NULL) {
            printf("CU_ADD_TEST fail. \n");
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_project.c");
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
