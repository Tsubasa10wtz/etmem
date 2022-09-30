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
 * Description: Cunit test for memdcd_cmd
 * **************************************************************************** */

#include <stdlib.h>
#include <errno.h>
#include <CUnit/Automated.h>
#include "memdcd_log.h"
#include "memdcd_message.h"
#include "memdcd_migrate.h"
#include "alloc_memory.h"

static void test_memdcd_cmd(void)
{
    struct memdcd_message *wrong_msg = NULL;
    int *msg_num = (int *)malloc(sizeof(int));
    if (msg_num == NULL) {
        printf("malloc error in test_memdcd_cmd");
        return;
    }
    wrong_msg = alloc_memory(100, msg_num);
    if (wrong_msg == NULL) {
        printf("alloc_memory return NULL in test_memdcd_cmd");
        free(msg_num);
        return;
    }
    int pid = wrong_msg->memory_msg.pid;
    CU_ASSERT_EQUAL(handle_recv_buffer((void *)wrong_msg, sizeof(struct memdcd_message) + 1), -1);
    CU_ASSERT_EQUAL(handle_recv_buffer((void *)wrong_msg, sizeof(struct memdcd_message) - 1), -1);
    wrong_msg->cmd_type = 100;
    CU_ASSERT_EQUAL(handle_recv_buffer((void *)wrong_msg, sizeof(struct memdcd_message)), -1);
    wrong_msg->cmd_type = MEMDCD_CMD_MEM;
    wrong_msg->memory_msg.pid = get_pid_max() + 1;
    CU_ASSERT_EQUAL(handle_recv_buffer((void *)wrong_msg, sizeof(struct memdcd_message)), -1);
    wrong_msg->memory_msg.pid = pid;
    wrong_msg->memory_msg.vma.total_length = 99999;
    CU_ASSERT_EQUAL(handle_recv_buffer((void *)wrong_msg, sizeof(struct memdcd_message)), -1);
    wrong_msg->memory_msg.vma.total_length = 100;
    wrong_msg->memory_msg.vma.length = sizeof(struct vma_addr_with_count) * 513;
    CU_ASSERT_EQUAL(handle_recv_buffer((void *)wrong_msg, sizeof(struct memdcd_message)), -1);
    free_memory(wrong_msg, *msg_num);
    free(msg_num);
}

int add_tests(void)
{
    /* add test case for memdcd_cmd */
    CU_pSuite suite_memdcd_cmd = CU_add_suite("memdcd_cmd", NULL, NULL);
    if (suite_memdcd_cmd == NULL) {
        return -1;
    }

    if (CU_ADD_TEST(suite_memdcd_cmd, test_memdcd_cmd) == NULL) {
        return -1;
    }

    CU_set_output_filename("memdcd");
    return 0;
}
