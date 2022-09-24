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
 * Description: Cunit test for memdcd_process
 * **************************************************************************** */

#include <stdlib.h>
#include <errno.h>
#include <CUnit/Automated.h>
#include "memdcd_process.c"
#include "alloc_memory.h"
#include "memdcd_policy.h"

static void test_memdcd_process(void)
{
    struct memdcd_message *wrong_msg = NULL;
    int *msg_num = (int *)malloc(sizeof(int));
    if (msg_num == NULL) {
        printf("malloc error in test_memdcd_process");
        return;
    }
    wrong_msg = alloc_memory(1023, msg_num);
    if (wrong_msg == NULL) {
        printf("alloc_memory return NULL in test_memdcd_process");
        free(msg_num);
        return;
    }
    init_mem_policy("./config/policy_threshold_GB.json");
    CU_ASSERT_EQUAL(migrate_process_get_pages(-1, wrong_msg->memory_msg.vma.vma_addrs), -1);
    CU_ASSERT_EQUAL(migrate_process_get_pages(get_pid_max() + 1, wrong_msg->memory_msg.vma.vma_addrs), -1);
    CU_ASSERT_EQUAL(migrate_process_get_pages(wrong_msg->memory_msg.pid, &(wrong_msg->memory_msg.vma)), -1);
    CU_ASSERT_EQUAL(migrate_process_get_pages(wrong_msg->memory_msg.pid, &(wrong_msg[1].memory_msg.vma)), 0);
    free_memory(wrong_msg, *msg_num);
    free(msg_num);
}

static void test_memdcd_process_static(void)
{
    int *msg_num = (int *)malloc(sizeof(int));
    struct memdcd_message *msg = NULL;
    if (msg_num == NULL) {
        printf("malloc error in test_memdcd_process_static");
        return;
    }
    init_mem_policy("./config/policy_threshold_80KB.json");
    msg = alloc_memory(1023, msg_num);
    if (msg == NULL) {
        printf("alloc_memory error in test_memdcd_process_static");
        free(msg_num);
        return;
    }
    struct migrate_process *process = migrate_process_collect_pages(getpid(), &(msg[0].memory_msg.vma));
    CU_ASSERT_PTR_NULL(process);
    process = migrate_process_collect_pages(getpid(), &(msg[1].memory_msg.vma));
    CU_ASSERT_PTR_NOT_NULL(process);
    CU_ASSERT_PTR_NOT_NULL(migrate_process_search(getpid()));
    migrate_process_free(process);

    CU_ASSERT_PTR_NULL(migrate_process_search(getpid()));
    process = migrate_process_collect_pages(getpid(), &(msg[0].memory_msg.vma));
    process = migrate_process_collect_pages(getpid(), &(msg[1].memory_msg.vma));
    migrate_process_remove(getpid());
    CU_ASSERT_PTR_NULL(migrate_process_search(getpid()));
    process = NULL;
    process = migrate_process_collect_pages(getpid(), &(msg[0].memory_msg.vma));
    process = migrate_process_collect_pages(getpid(), &(msg[1].memory_msg.vma));
    CU_ASSERT_PTR_NOT_NULL(process);
    memdcd_migrate(NULL);
    process->pid = get_pid_max() + 1;
    memdcd_migrate(process);
    CU_ASSERT_PTR_NULL(migrate_process_search(getpid()));
    process->pid = getpid();
    process = NULL;
    migrate_process_add(getpid(), 100);
    process = migrate_process_search(getpid());
    CU_ASSERT_PTR_NOT_NULL(process);
    CU_ASSERT_EQUAL(process->page_list->length, 100);
    migrate_process_free(process);
    free_memory(msg, *msg_num);
    free(msg_num);
}

int add_tests(void)
{
    /* add test case for interface of memdcd_process */
    CU_pSuite suite_memdcd_process = CU_add_suite("memdcd_process", NULL, NULL);
    if (suite_memdcd_process == NULL) {
        return -1;
    }

    if (CU_ADD_TEST(suite_memdcd_process, test_memdcd_process) == NULL) {
        return -1;
    }

    /* add test case for static func of memdcd_process */
    CU_pSuite suite_memdcd_process_static = CU_add_suite("memdcd_process_static", NULL, NULL);
    if (suite_memdcd_process == NULL) {
        return -1;
    }

    if (CU_ADD_TEST(suite_memdcd_process_static, test_memdcd_process_static) == NULL) {
        return -1;
    }

    CU_set_output_filename("memdcd");
    return 0;
}
