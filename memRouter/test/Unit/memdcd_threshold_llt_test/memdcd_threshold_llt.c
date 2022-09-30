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
 * Description: Cunit test for memdcd_threshold
 * **************************************************************************** */

#include <stdlib.h>
#include <errno.h>
#include <CUnit/Automated.h>
#include "memdcd_policy.h"
#include "memdcd_policy_threshold.h"
#include "memdcd_process.h"
#include "alloc_memory.h"
static void *fake_malloc(size_t size)
{
    return NULL;
}
static void test_memdcd_threshlod(void)
{
    int k = 0;
    struct migrate_page_list *pages = NULL, *pages_to_numa = NULL, *pages_to_swap = NULL;
    struct mem_policy *policy = NULL;
    struct memdcd_message *msg = NULL;
    struct memdcd_policy_opt *threshold_policy_opt = NULL;
    int *msg_num = (int *)malloc(sizeof(int));
    if (msg_num == NULL) {
        printf("malloc error in test_memdcd_threshlod");
        return;
    }
    policy = (struct mem_policy *)malloc(sizeof(struct mem_policy));
    if (policy == NULL) {
        free(msg_num);
        printf("malloc error in test_memdcd_threshlod");
        return;
    }
    policy->private = NULL;
    threshold_policy_opt = get_threshold_policy();
    CU_ASSERT_EQUAL(threshold_policy_opt->destroy(NULL), -1);
    CU_ASSERT_EQUAL(threshold_policy_opt->init(policy, "hviehigfhi8"), -1);
    CU_ASSERT_EQUAL(threshold_policy_opt->destroy(policy), -1);
    CU_ASSERT_EQUAL(threshold_policy_opt->init(policy, "./config/policy_threshold_invalid1.json"), -1);
    CU_ASSERT_EQUAL(threshold_policy_opt->destroy(policy), -1);
    CU_ASSERT_EQUAL(threshold_policy_opt->init(policy, "./config/policy_threshold_invalid2.json"), -1);
    CU_ASSERT_EQUAL(threshold_policy_opt->destroy(policy), -1);

    CU_ASSERT_EQUAL(threshold_policy_opt->init(policy, "./config/policy_threshold.json"), 0);
    CU_ASSERT_EQUAL(threshold_policy_opt->destroy(policy), 0);
    CU_ASSERT_EQUAL(threshold_policy_opt->init(policy, "./config/policy_threshold.json"), 0);
    pages = (struct migrate_page_list *)malloc(sizeof(struct migrate_page) * 1023 + sizeof(struct migrate_page_list));
    if (pages == NULL) {
        free(msg_num);
        free(policy);
        printf("malloc error in test_memdcd_threshlod");
        return;
    }
    memset(pages, 15, sizeof(sizeof(struct migrate_page) * 1023 + sizeof(struct migrate_page_list)));
    pages->length = 1023;
    CU_ASSERT_EQUAL(threshold_policy_opt->parse(policy, getpid(), pages, &pages_to_numa, &pages_to_swap), 0);
    CU_ASSERT_PTR_NULL(pages_to_numa);
    CU_ASSERT_PTR_NOT_NULL(pages_to_swap);
    CU_ASSERT_EQUAL(pages_to_swap->length, 0);
    msg = alloc_memory(1023, msg_num);
    if (msg == NULL) {
        free(msg_num);
        free(policy);
        free(pages);
        printf("alloc_memory return error in test_memdcd_threshlod");
        return;
    }
    for (int i = 0; i < *msg_num; i++) {
        for (int j = 0; j < 512 && k < 1023; j++) {
            pages->pages[k].addr = msg[i].memory_msg.vma.vma_addrs[j].vma.start_addr;
            pages->pages[k].length = msg[i].memory_msg.vma.vma_addrs[j].vma.vma_len;
            k++;
        }
    }
    CU_ASSERT_EQUAL(threshold_policy_opt->parse(policy, getpid(), pages, &pages_to_numa, &pages_to_swap), 0);
    CU_ASSERT_PTR_NOT_NULL(pages_to_swap);
    CU_ASSERT(pages_to_swap->length > 0);
    free_memory(msg, *msg_num);
    free(msg_num);
    free(policy);
    free(pages);
}

int add_tests(void)
{
    /* add test case for memdcd_thread */
    CU_pSuite suite_memdcd_threshlod = CU_add_suite("memdcd_threshlod", NULL, NULL);
    if (suite_memdcd_threshlod == NULL) {
        return -1;
    }

    if (CU_ADD_TEST(suite_memdcd_threshlod, test_memdcd_threshlod) == NULL) {
        return -1;
    }

    CU_set_output_filename("memdcd");
    return 0;
}
