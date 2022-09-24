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
 * Description: Cunit test for memdcd_migrate
 * **************************************************************************** */

#include <stdlib.h>
#include <errno.h>
#include <CUnit/Automated.h>
#include <pthread.h>
#include "memdcd_migrate.c"
#include "memdcd_message.h"
#include "alloc_memory.h"

void *uswap_socket(void *recv_msg)
{
    int pid = getpid();
    struct vma_addr *msg = (struct vma_addr *)recv_msg;
    simulate_uswap(pid, &msg);
    return recv_msg;
}

static void test_memdcd_migrate(void)
{
    struct migrate_page_list *page_list = NULL;
    struct vma_addr *recv_msg = NULL;
    struct memdcd_message *msg = NULL;
    pthread_t uswap;
    int *msg_num = (int *)malloc(sizeof(int));
    if (msg_num == NULL) {
        printf("malloc error in test_memdcd_migrate");
        return;
    }
    page_list = (struct migrate_page_list *)malloc(sizeof(struct migrate_page) * 511 + sizeof(struct migrate_page_list));
    if (page_list == NULL) {
        printf("malloc error in test_memdcd_migrate");
        free(msg_num);
        return;
    }
    memset(page_list, 0, sizeof(sizeof(struct migrate_page) * 511 + sizeof(struct migrate_page_list)));

    msg = alloc_memory(511, msg_num);
    if (msg == NULL) {
        printf("alloc_memory return error in test_memdcd_migrate");
        free(msg_num);
        free(page_list);
        return;
    }
    page_list->length = 511;
    int k = 0;
    for (int i = 0; i < *msg_num; i++) {
        for (int j = 0; j < 511 && k < 511; j++) {
            page_list->pages[k].addr = msg[i].memory_msg.vma.vma_addrs[i].vma.start_addr;
            page_list->pages[k].length = msg[i].memory_msg.vma.vma_addrs[i].vma.vma_len;
            k++;
        }
    }

    CU_ASSERT_EQUAL(uswap_init_connection(getpid(), CLIENT_RECV_DEFAULT_TIME), -1);
    pthread_create(&uswap, NULL, uswap_socket, (void *)recv_msg);
    int detect_sock = uswap_init_connection(getpid(), CLIENT_RECV_DEFAULT_TIME);
    while (detect_sock < 0) {
        detect_sock = uswap_init_connection(getpid(), CLIENT_RECV_DEFAULT_TIME);
    }
    close(detect_sock);
    CU_ASSERT_EQUAL(send_to_userswap(getpid(), page_list), 0);
    free(page_list);
    free_memory(msg, *msg_num);
    free(msg_num);
}

int add_tests(void)
{
    /* add test case for memdcd_migrate */
    CU_pSuite suite_memdcd_migrate = CU_add_suite("memdcd_migrate", NULL, NULL);
    if (suite_memdcd_migrate == NULL) {
        return -1;
    }

    if (CU_ADD_TEST(suite_memdcd_migrate, test_memdcd_migrate) == NULL) {
        return -1;
    }

    CU_set_output_filename("memdcd");
    return 0;
}
