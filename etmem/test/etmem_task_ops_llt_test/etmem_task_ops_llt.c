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
 * Description: This is a source file of the unit test for task functions in etmem.
 ******************************************************************************/

#include <sys/file.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "etmemd_project.h"
#include "etmemd_task.h"
#include "etmemd_engine.h"
#include "etmemd_slide.h"
#include "etmemd_cslide.h"
#include "etmemd_rpc.h"
#include "securec.h"

#define PID_STR_MAX_LEN         10
#define PID_TEST_NUM            100
#define PID_PROCESS_MEM         5000
#define PID_PROCESS_SLEEP_TIME  60
#define WATER_LINT_TEMP         3

static void get_task_pids_errinput(char *pid_val, char *pid_type, int exp)
{
    struct task *tk = NULL;

    tk = (struct task *)calloc(1, sizeof(struct task));
    CU_ASSERT_PTR_NOT_NULL(tk);

    tk->type = pid_type;
    tk->value = pid_val;
    CU_ASSERT_EQUAL(etmemd_get_task_pids(tk, true), exp);

    free(tk);
}

static void test_get_task_pids_error(void)
{
    get_task_pids_errinput("", "", -1);
    get_task_pids_errinput("1", "", -1);
    get_task_pids_errinput("no123", "pid", -1);
    get_task_pids_errinput("1", "wrong", -1);
    get_task_pids_errinput("wrong", "name", -1);
    get_task_pids_errinput("-1", "pid", -1);
}

static void add_process(int index)
{
    int pid = fork();
    switch (pid) {
        case 0: /* child process */
        {
            char index_str[PID_PROCESS_MEM] = {0};
            if (snprintf_s(index_str, PID_PROCESS_MEM, PID_PROCESS_MEM - 1, "%d", index) <= 0) {
                printf("get index error.\n");
            }
            prctl(PR_SET_NAME, index_str, NULL, NULL, NULL);
            sleep(PID_PROCESS_SLEEP_TIME);
            exit(0);
        }
        case -1: /* create process wrong */
        {
            printf("[Worker]: Fork failed!\n");
            exit(0);
        }
    
        default:
            break;
    }
}

static int get_pids(int index)
{
    int pid = getpid();
    int i;
    for (i = 0; i < index; i++) {
        add_process(i);
    }
    return pid;
}

static int init_ops(struct task *tk)
{
    tk->eng->ops->fill_eng_params = NULL;
    tk->eng->ops->clear_eng_params = NULL;
    tk->eng->ops->fill_task_params = NULL;
    tk->eng->ops->clear_task_params = NULL;
    tk->eng->ops->start_task = NULL;
    tk->eng->ops->stop_task = NULL;
    tk->eng->ops->alloc_pid_params = NULL;
    tk->eng->ops->free_pid_params = NULL;
    tk->eng->ops->eng_mgt_func = NULL;
}

static struct task *alloc_task(const char *pid_type, const char *pid_val)
{
    size_t pid_type_len;
    size_t pid_value_len;
    struct task *tk = NULL;

    tk = (struct task *)calloc(1, sizeof(struct task));
    CU_ASSERT_PTR_NOT_NULL(tk);

    pid_type_len = strlen(pid_type) + 1;
    pid_value_len = strlen(pid_val) + 1;

    tk->type = (char *)calloc(pid_type_len, sizeof(char));
    CU_ASSERT_PTR_NOT_NULL(tk->type);
    if (strncpy_s(tk->type, pid_type_len, pid_type, pid_type_len - 1) != EOK) {
        free(tk->type);
        free(tk);
        return NULL;
    }

    tk->value = (char *)calloc(pid_value_len, sizeof(char));
    CU_ASSERT_PTR_NOT_NULL(tk->value);
    if (strncpy_s(tk->value, pid_value_len, pid_val, pid_value_len - 1) != EOK) {
        free(tk->type);
        free(tk->value);
        free(tk);
        return NULL;
    }

    tk->eng = (struct engine *)calloc(1, sizeof(struct engine));
    CU_ASSERT_PTR_NOT_NULL(tk->eng);
    tk->eng->tasks = (void *)tk;

    tk->eng->ops = (struct engine_ops *)calloc(1, sizeof(struct engine_ops));
    CU_ASSERT_PTR_NOT_NULL(tk->eng->ops);
    init_ops(tk);

    CU_ASSERT_EQUAL(fill_engine_type_slide(tk->eng, NULL), 0);

    return tk;
}

static void test_get_task_withpid_ok(void)
{
    char pid_val[PID_STR_MAX_LEN] = {0};
    char *pid_type = "pid";
    struct task *tk = NULL;
    struct engine *eng = NULL;
    int pid;

    pid = get_pids((int)PID_TEST_NUM);
    if (snprintf_s(pid_val, PID_STR_MAX_LEN, PID_STR_MAX_LEN - 1, "%d", pid) <= 0) {
        printf("snprintf pid fail %d", pid);
        return;
    }
    tk = alloc_task(pid_type, pid_val);
    CU_ASSERT_PTR_NOT_NULL(tk);

    CU_ASSERT_EQUAL(etmemd_get_task_pids(tk, true), 0);
    CU_ASSERT_PTR_NOT_NULL(tk->pids);

    etmemd_free_task_pids(tk);
    CU_ASSERT_PTR_NULL(tk->pids);

    etmemd_free_task_struct(&tk);
    CU_ASSERT_PTR_NULL(tk);
}

static void test_get_task_withname_ok(void)
{
    char *pid_val = "systemd";
    char *pid_type = "name";
    struct task *tk = NULL;

    tk = alloc_task(pid_type, pid_val);

    CU_ASSERT_EQUAL(etmemd_get_task_pids(tk, true), 0);
    CU_ASSERT_PTR_NOT_NULL(tk->pids);

    etmemd_free_task_pids(tk);
    CU_ASSERT_PTR_NULL(tk->pids);

    etmemd_free_task_struct(&tk);
    CU_ASSERT_PTR_NULL(tk);
}

static int get_task_pid(char *type, char *value, struct task *tk)
{
    char pid[PID_STR_MAX_LEN] = {0};
    tk->type = type;
    tk->value = value;
    return get_pid_from_task_type(tk, pid);
}

static void test_get_pid_error(void)
{
    struct task *tk = NULL;

    tk = (struct task *)calloc(1, sizeof(struct task));
    CU_ASSERT_PTR_NOT_NULL(tk);

    CU_ASSERT_EQUAL(get_task_pid("", "", tk), -1);
    CU_ASSERT_EQUAL(get_task_pid("wrong", "", tk), -1);
    CU_ASSERT_EQUAL(get_task_pid("", "no123", tk), -1);
    CU_ASSERT_EQUAL(get_task_pid("pid", "no123", tk), 0);
    CU_ASSERT_EQUAL(get_task_pid("name", "wrong", tk), -1);
    free(tk);
}

static void test_get_pid_ok(void)
{
    struct task *tk = NULL;

    tk = (struct task *)calloc(1, sizeof(struct task));
    CU_ASSERT_PTR_NOT_NULL(tk);

    CU_ASSERT_EQUAL(get_task_pid("pid", "1", tk), 0);
    CU_ASSERT_EQUAL(get_task_pid("name", "systemd", tk), 0);
    free(tk);
}

static void test_free_task_pids(void)
{
    struct task *tk = NULL;
    struct task_pid *tk_pid = NULL;
    struct slide_params *s_param = NULL;
    struct engine *eng = NULL;

    tk = (struct task *)calloc(1, sizeof(struct task));
    CU_ASSERT_PTR_NOT_NULL(tk);

    tk_pid = (struct task_pid *)calloc(1, sizeof(struct task_pid));
    CU_ASSERT_PTR_NOT_NULL(tk_pid);
    tk_pid->pid = 1;

    s_param = (struct slide_params *)calloc(1, sizeof(struct slide_params));
    CU_ASSERT_PTR_NOT_NULL(s_param);
    s_param->t = WATER_LINT_TEMP;
    tk_pid->params = s_param;
    tk_pid->tk = tk;
    tk->pids = tk_pid;

    /* add engine */
    eng = (struct engine *)calloc(1, sizeof(struct engine));
    CU_ASSERT_PTR_NOT_NULL(eng);
    tk->eng = eng;

    tk->eng->ops = (struct engine_ops *)calloc(1, sizeof(struct engine_ops));
    CU_ASSERT_PTR_NOT_NULL(tk->eng->ops);
    init_ops(tk);

    etmemd_free_task_pids(tk);
    CU_ASSERT_PTR_NULL(tk->pids);
    free(tk);
    tk = NULL;
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

    suite = CU_add_suite("etmem_task_ops", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_get_task_pids_error) == NULL ||
        CU_ADD_TEST(suite, test_get_task_withpid_ok) == NULL ||
        CU_ADD_TEST(suite, test_get_task_withname_ok) == NULL ||
        CU_ADD_TEST(suite, test_get_pid_error) == NULL ||
        CU_ADD_TEST(suite, test_get_pid_ok) == NULL ||
        CU_ADD_TEST(suite, test_free_task_pids) == NULL) {
            printf("CU_ADD_TEST fail. \n");
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_task.c");
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
