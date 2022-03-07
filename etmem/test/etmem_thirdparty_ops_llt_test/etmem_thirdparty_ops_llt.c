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
 * Create: 2021-12-10
 * Description: This is a source file of the unit test for thirdparty functions in etmem.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "test_common.h"
#include "etmemd_thirdparty_export.h"
#include "etmemd_project.h"

#define CONFIG_ENGINE_NAME          "eng_name=%s\n"
#define CONFIG_LIBNAME              "libname=%s\n"
#define CONFIG_OPS_NAME             "ops_name=%s\n"
#define CONFIG_ENGINE_PRIVATE_KEY   "engine_private_key=%s\n"

#define CONFIG_TASK_PRIVATE_KEY     "task_private_key=%s\n"

struct thirdparty_eng_param {
    struct eng_test_param eng_param;
    const char *engine_name;
    const char *libname;
    const char *ops_name;
    const char *engine_private_key;
};

struct thirdparty_task_param {
    struct task_test_param task_param;
    const char *task_private_key;
};

static struct proj_test_param g_proj_param;

static void init_thirdparty_eng(struct thirdparty_eng_param *param, 
                                const char *libname, const char *ops_name)
{
    param->eng_param.name = "thirdparty";
    param->eng_param.proj = DEFAULT_PROJ;
    param->eng_param.file_name = TMP_TASK_CONFIG;
    param->engine_name = "my_engine";
    param->libname = libname;
    param->ops_name = ops_name;
    param->engine_private_key = "1024";
}

static void add_thirdparty_eng(struct thirdparty_eng_param *param)
{
    FILE *file = NULL;
    const char *file_name = param->eng_param.file_name;

    file = fopen(file_name, "a+");
    CU_ASSERT_PTR_NOT_NULL(file);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_ENGINE_NAME, param->engine_name), -1);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_LIBNAME, param->libname), -1);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_OPS_NAME, param->ops_name), -1);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_ENGINE_PRIVATE_KEY, param->engine_private_key), -1);
    fclose(file);
}

static GKeyFile *construct_thirdparty_eng_config(struct thirdparty_eng_param *param)
{
    struct eng_test_param *eng_param = &param->eng_param;

    construct_eng_file(eng_param);
    add_thirdparty_eng(param);

    return load_config(eng_param->file_name);
}

static void destroy_thirdparty_eng_config(GKeyFile *config)
{
    unload_config(config);
}

static void init_thirdparty_task(struct thirdparty_task_param *param)
{
    init_task_param(&param->task_param, "my_engine");
    param->task_private_key = "2048";
}

static void add_thirdparty_task(struct thirdparty_task_param *param)
{
    FILE *file = NULL;
    const char *file_name = param->task_param.file_name;

    file = fopen(file_name, "a+");
    CU_ASSERT_PTR_NOT_NULL(file);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_TASK_PRIVATE_KEY, param->task_private_key), -1);
    fclose(file);
}

static GKeyFile *construct_thirdparty_task_config(struct thirdparty_task_param *param)
{
    struct task_test_param *task_param = &param->task_param;

    construct_task_file(task_param);
    add_thirdparty_task(param);

    return load_config(task_param->file_name);
}

static void destroy_thirdparty_task_config(GKeyFile *config)
{
    unload_config(config);
}

static void test_init(void)
{
    init_proj_param(&g_proj_param);
    do_add_proj_test(&g_proj_param);
}

static void test_fini(void)
{
    do_rm_proj_test(&g_proj_param);
}

static void test_etmem_mgt_engine_error(void)
{
    CU_ASSERT_EQUAL(etmemd_project_mgt_engine(NULL, NULL, NULL, DEFAULT_TASK, 0), OPT_INVAL);
}

void test_etmem_user_engine_0001(void)
{
    struct thirdparty_eng_param thirdparty_engine_param;
    struct thirdparty_task_param thirdparty_task_param;
    GKeyFile *eng_config = NULL;
    GKeyFile *task_config = NULL;

    test_init();

    init_thirdparty_eng(&thirdparty_engine_param, "./lib/libthirdparty_engine.so", "my_engine_ops");
    init_thirdparty_task(&thirdparty_task_param);

    eng_config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(eng_config), OPT_SUCCESS);

    task_config = construct_thirdparty_task_config(&thirdparty_task_param);
    CU_ASSERT_EQUAL(etmemd_project_add_task(task_config), OPT_SUCCESS);

    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_mgt_engine(DEFAULT_PROJ, "my_engine", 
                                              "my_cmd", DEFAULT_TASK, 0), OPT_SUCCESS);

    /* test param NULL */
    CU_ASSERT_EQUAL(etmemd_project_mgt_engine(NULL, NULL, 
                                              NULL, DEFAULT_TASK, 0), OPT_INVAL);
    CU_ASSERT_EQUAL(etmemd_project_mgt_engine(DEFAULT_PROJ, "no_exist_engine", 
                                              "my_cmd", DEFAULT_TASK, 0), OPT_ENG_NOEXIST);
    CU_ASSERT_EQUAL(etmemd_project_mgt_engine(DEFAULT_PROJ, "my_engine", 
                                              "my_cmd", "no_exist_task", 0), OPT_TASK_NOEXIST);

    CU_ASSERT_EQUAL(etmemd_migrate_stop(DEFAULT_PROJ), OPT_SUCCESS);

    /* test for project not start */
    CU_ASSERT_EQUAL(etmemd_project_mgt_engine(DEFAULT_PROJ, "my_engine", 
                                              "my_cmd", DEFAULT_TASK, 0), OPT_INVAL);

    CU_ASSERT_EQUAL(etmemd_project_remove_task(task_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(eng_config), OPT_SUCCESS);

    destroy_thirdparty_task_config(task_config);
    destroy_thirdparty_eng_config(eng_config);

    init_thirdparty_eng(&thirdparty_engine_param, "./lib/libthirdparty_engine.so", "start_error_ops");
    init_thirdparty_task(&thirdparty_task_param);

    eng_config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(eng_config), OPT_SUCCESS);

    task_config = construct_thirdparty_task_config(&thirdparty_task_param);
    /* start task fail when start projecct */
    CU_ASSERT_EQUAL(etmemd_project_add_task(task_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_INTER_ERR);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(task_config), OPT_SUCCESS);

    /* add task fail if project is already start */
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_add_task(task_config), OPT_INTER_ERR);

    destroy_thirdparty_task_config(task_config);
    destroy_thirdparty_eng_config(eng_config);

    test_fini();
}

static void test_etmem_user_engine_null(void)
{
    struct thirdparty_eng_param thirdparty_engine_param;
    struct thirdparty_task_param thirdparty_task_param;
    GKeyFile *eng_config = NULL;
    GKeyFile *task_config = NULL;

    test_init();

    init_thirdparty_eng(&thirdparty_engine_param, "./lib/libthirdparty_engine.so", "start_not_null_ops");
    init_thirdparty_task(&thirdparty_task_param);

    eng_config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    task_config = construct_thirdparty_task_config(&thirdparty_task_param);

    /* only start task is not null to cover null mgt and stop */
    CU_ASSERT_EQUAL(etmemd_project_add_engine(eng_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_add_task(task_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    CU_ASSERT_NOT_EQUAL(etmemd_project_mgt_engine(DEFAULT_PROJ, "my_engine", 
                                              "my_cmd", DEFAULT_TASK, 0), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_stop(DEFAULT_PROJ), OPT_SUCCESS);

    CU_ASSERT_EQUAL(etmemd_project_remove_task(task_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(eng_config), OPT_SUCCESS);

    destroy_thirdparty_task_config(task_config);
    destroy_thirdparty_eng_config(eng_config);

    test_fini();

    test_init();

    init_thirdparty_eng(&thirdparty_engine_param, "./lib/libthirdparty_engine.so", "start_null_ops");
    init_thirdparty_task(&thirdparty_task_param);

    eng_config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    task_config = construct_thirdparty_task_config(&thirdparty_task_param);

    /* start task manually */
    CU_ASSERT_EQUAL(etmemd_project_add_engine(eng_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_add_task(task_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_INTER_ERR);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(task_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(eng_config), OPT_SUCCESS);

    /* start task auto (project is already start when add a task) */
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(eng_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_add_task(task_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(eng_config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_stop(DEFAULT_PROJ), OPT_SUCCESS);

    destroy_thirdparty_task_config(task_config);
    destroy_thirdparty_eng_config(eng_config);

    test_fini();
}

static void test_etmem_invalid_config(void)
{
    struct thirdparty_eng_param thirdparty_engine_param;
    GKeyFile *eng_config = NULL;

    test_init();

    /* test invalid so */
    init_thirdparty_eng(&thirdparty_engine_param, "./lib/invalid.so", "my_engine_ops");
    eng_config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(eng_config), OPT_SUCCESS);
    destroy_thirdparty_eng_config(eng_config);

    /* test invalid engine name */
    init_thirdparty_eng(&thirdparty_engine_param, "./lib/libthirdparty_engine.so", "my_engine_ops");
    thirdparty_engine_param.engine_name = "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
    eng_config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(eng_config), OPT_SUCCESS);
    destroy_thirdparty_eng_config(eng_config);

    /* test invalid ops */
    init_thirdparty_eng(&thirdparty_engine_param, "./lib/libthirdparty_engine.so", "no_exist_ops");
    eng_config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(eng_config), OPT_SUCCESS);
    destroy_thirdparty_eng_config(eng_config);

    test_fini();
}

void test_etmem_user_engine_0002(void)
{
    test_etmem_user_engine_null();
}

/* add correct thirdparty config */
void test_etmem_add_thirdparty_0001(void)
{
    struct thirdparty_eng_param thirdparty_engine_param;
    GKeyFile *config = NULL;

    test_init();

    init_thirdparty_eng(&thirdparty_engine_param, "./lib/libthirdparty_engine.so", "my_engine_ops");
    config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_thirdparty_eng_config(config);

    test_fini();
}

/* add wrong thirdparty config */
void test_etmem_add_thirdparty_0002(void)
{
    test_etmem_invalid_config();
}

/* del correct thirdparty config */
void test_etmem_del_thirdparty_0001(void)
{
    struct thirdparty_eng_param thirdparty_engine_param;
    GKeyFile *config = NULL;

    test_init();

    init_thirdparty_eng(&thirdparty_engine_param, "./lib/libthirdparty_engine.so", "my_engine_ops");
    config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_thirdparty_eng_config(config);

    test_fini();
}

/* del wrong thirdparty config */
void test_etmem_del_thirdparty_0002(void)
{
    struct thirdparty_eng_param thirdparty_engine_param;
    GKeyFile *config = NULL;

    test_init();

    /* del no exist engine */
    init_thirdparty_eng(&thirdparty_engine_param, "./lib/libthirdparty_engine.so", "my_engine_ops");
    config = construct_thirdparty_eng_config(&thirdparty_engine_param);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_ENG_NOEXIST);
    destroy_thirdparty_eng_config(config);

    test_fini();
}

void do_rm_proj_test(struct proj_test_param *param)
{
    GKeyFile *config = NULL;

    config = load_config(param->file_name);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), param->expt);
    unload_config(config);
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

    suite = CU_add_suite("etmem_thirdparty_ops", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_etmem_user_engine_0001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_user_engine_0002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_add_thirdparty_0001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_add_thirdparty_0002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_del_thirdparty_0001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_del_thirdparty_0002) == NULL) {
            printf("CU_ADD_TEST fail. \n");
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_thirdparty.c");
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
