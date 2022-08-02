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
 * Create: 2021-08-14
 * Description: This is a source file of the unit test for rpc functions in etmem.
 ******************************************************************************/

#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "etmem_common.h"
#include "etmem_rpc.h"
#include "etmemd_rpc.h"
#include "securec.h"
#include "etmem.h"

#define SOCK_NAME_MAX_LEN       108
#define RPC_SEND_FILE_MAX       512
#define SLEEP_TIME_ONE_T_US     1000

static char *get_length_str(unsigned int length)
{
    char *result = NULL;
    unsigned char i = 'a';
    unsigned int j;
    result = (char *)calloc(length + 1, sizeof(char));
    if (result == NULL) {
        return NULL;
    }

    for (j = 0; j < length; j++) {
        result[j] = i;
    }

    return result;
}

static void test_sock_name_error(void)
{
    char *name = NULL;
    CU_ASSERT_EQUAL(etmemd_parse_sock_name(NULL), -1);
    CU_ASSERT_EQUAL(etmemd_parse_sock_name(""), -1);
    CU_ASSERT_EQUAL(etmemd_parse_sock_name("name"), 0);

    name = get_length_str(SOCK_NAME_MAX_LEN - 1);
    CU_ASSERT_PTR_NOT_NULL(name);
    CU_ASSERT_EQUAL(etmemd_parse_sock_name(name), -1);
    free(name);

    name = get_length_str(SOCK_NAME_MAX_LEN);
    CU_ASSERT_PTR_NOT_NULL(name);
    CU_ASSERT_EQUAL(etmemd_parse_sock_name(name), -1);
    free(name);

    name = get_length_str(SOCK_NAME_MAX_LEN + 1);
    CU_ASSERT_PTR_NOT_NULL(name);
    CU_ASSERT_EQUAL(etmemd_parse_sock_name(name), -1);
    free(name);
}

static void test_sock_name_ok(void)
{
    char *name = NULL;

    CU_ASSERT_EQUAL(etmemd_parse_sock_name("name"), 0);
    CU_ASSERT_EQUAL(etmemd_parse_sock_name("n"), 0);

    name = get_length_str(SOCK_NAME_MAX_LEN - 2);
    CU_ASSERT_PTR_NOT_NULL(name);
    CU_ASSERT_EQUAL(etmemd_parse_sock_name(name), 0);
    free(name);
}

void *etmemd_rpc_server_start(void *msg)
{
    char *socket_name = (char *)msg;
    CU_ASSERT_EQUAL(etmemd_parse_sock_name(socket_name), -1);
    etmemd_rpc_server();
}

void *etmemd_rpc_server_start_ok(void *msg)
{
    char *socket_name = (char *)msg;
    CU_ASSERT_EQUAL(etmemd_parse_sock_name(socket_name), 0);
    etmemd_rpc_server();
}

static struct mem_proj *alloc_proj(int cmd, char *proj_name,
                                   char *file_name, char *sock_name)
{
    struct mem_proj *proj = NULL;
    proj = (struct mem_proj *)calloc(1, sizeof(struct mem_proj));
    CU_ASSERT_PTR_NOT_NULL(proj);

    proj->cmd = cmd;
    proj->file_name = file_name;
    proj->proj_name = proj_name;
    proj->sock_name = sock_name;

    return proj;
}

static int etmem_socket_client(int cmd, char *proj_name,
                               char *file_name, char *sock_name)
{
    int ret;
    struct mem_proj *proj = NULL;
    proj = alloc_proj(cmd, proj_name, file_name, sock_name);
    ret = etmem_rpc_client(proj);
    free(proj);
    if (ret == 0) {
        return 0;
    }
    return -1;
}

static void etmem_pro_add_error(void)
{
    char *file_name = NULL;
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, NULL, NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, NULL, "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, NULL, "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, NULL, file_name, ""), -1);
    free(file_name);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, NULL, "file_name", "sock_name"), -1);

    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "", NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "", "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "", "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "", file_name, ""), -1);
    free(file_name);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "", "file_name", "sock_name"), -1);

    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "test", NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "test", "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "test", "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "test", file_name, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "test", file_name, "sock_name"), -1);
    free(file_name);
}

static void etmem_pro_del_error(void)
{
    char *file_name = NULL;
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, NULL, NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, NULL, "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, NULL, "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, NULL, file_name, ""), -1);
    free(file_name);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, NULL, "file_name", "sock_name"), -1);

    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "", NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "", "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "", "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "", file_name, ""), -1);
    free(file_name);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "", "file_name", "sock_name"), -1);

    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "test", NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "test", "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "test", "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "test", file_name, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "test", file_name, "sock_name"), -1);
    free(file_name);
}

static void etmem_pro_start_error(void)
{
    char *file_name = NULL;
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, NULL, NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, NULL, "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, NULL, "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, NULL, file_name, ""), -1);
    free(file_name);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, NULL, "file_name", "sock_name"), -1);

    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "", NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "", "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "", "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "", file_name, ""), -1);
    free(file_name);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "", "file_name", "sock_name"), -1);

    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "test", NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "test", "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "test", "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "test", file_name, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "test", file_name, "sock_name"), -1);
    free(file_name);
}

static void etmem_pro_stop_error(void)
{
    char *file_name = NULL;
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, NULL, NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, NULL, "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, NULL, "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, NULL, file_name, ""), -1);
    free(file_name);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, NULL, "file_name", "sock_name"), -1);

    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "", NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "", "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "", "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "", file_name, ""), -1);
    free(file_name);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "", "file_name", "sock_name"), -1);

    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "test", NULL, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "test", "", ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "test", "file_name", ""), -1);
    file_name = get_length_str(RPC_SEND_FILE_MAX);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "test", file_name, ""), -1);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "test", file_name, "sock_name"), -1);
    free(file_name);
}

static void test_socket_client_error(void)
{
    struct mem_proj *proj = NULL;
    char *project_name = NULL;
    char *file_name = NULL;
    char *sock_name = NULL;
    char *msg = "etmem_error_sock";

    pthread_t etmem_socket_test;

    CU_ASSERT_EQUAL(pthread_create(&etmem_socket_test, NULL, etmemd_rpc_server_start, NULL), EOK);
    CU_ASSERT_EQUAL(pthread_create(&etmem_socket_test, NULL,
                                   etmemd_rpc_server_start_ok, (void *)msg), EOK);
    usleep(SLEEP_TIME_ONE_T_US);
    etmem_pro_add_error();
    etmem_pro_del_error();
    etmem_pro_start_error();
    etmem_pro_stop_error();

    etmemd_handle_signal();
}

static void test_socket_client_ok(void)
{
    struct mem_proj *proj = NULL;
    char *project_name = NULL;
    char *file_name = NULL;
    char *sock_name = NULL;
    char *msg = "etmem_ok_sock";

    pthread_t etmem_socket_test_ok;

    CU_ASSERT_EQUAL(pthread_create(&etmem_socket_test_ok, NULL,
                                   etmemd_rpc_server_start_ok, (void *)msg), EOK);
    usleep(SLEEP_TIME_ONE_T_US);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_ADD, "test",
                                        "../conf/conf_slide/config_file", msg), 0);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_SHOW, "test",
                                        "../conf/conf_slide/config_file", msg), 0);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_START, "test",
                                        "../conf/conf_slide/config_file", msg), 0);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_STOP, "test",
                                        "../conf/conf_slide/config_file", msg), 0);
    CU_ASSERT_EQUAL(etmem_socket_client(ETMEM_CMD_DEL, "test",
                                        "../conf/conf_slide/config_file", msg), 0);

    etmemd_handle_signal();
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

    suite = CU_add_suite("etmem_socket_ops", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_sock_name_error) == NULL ||
        CU_ADD_TEST(suite, test_sock_name_ok) == NULL ||
        CU_ADD_TEST(suite, test_socket_client_error) == NULL ||
        CU_ADD_TEST(suite, test_socket_client_ok) == NULL) {
            printf("CU_ADD_TEST fail. \n");
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_rpc.c");
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
