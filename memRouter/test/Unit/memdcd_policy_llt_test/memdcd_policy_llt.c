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
 * Description: Cunit test for memdcd_policy
 * **************************************************************************** */

#include <stdlib.h>
#include <errno.h>
#include <CUnit/Automated.h>
#include <sys/stat.h>
#include "memdcd_policy.h"
#include "alloc_memory.h"

static void test_memdcd_policy(void)
{
    CU_ASSERT_EQUAL(init_mem_policy("feifjijijfe"), -EEXIST);
    chmod("./config/policy_threshold_invalid1.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_invalid1.json"), -1);

    chmod("./config/policy_threshold_invalid2.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_invalid2.json"), -1)

    chmod("./config/policy_threshold.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold.json"), 0);

    chmod("./config/policy_threshold_MB.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_MB.json"), 0);

    chmod("./config/policy_threshold_GB.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_GB.json"), 0);

    chmod("./config/policy_threshold_invalid_unit.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_invalid_unit.json"), -1);

    chmod("./config/policy_threshold_invalid_key_policy.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_invalid_key_policy.json"), -1);

    chmod("./config/policy_threshold_invalid_key_unit.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_invalid_key_unit.json"), -1);

    chmod("./config/policy_threshold_invalid_key_threshold.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_invalid_key_threshold.json"), -1);

    chmod("./config/policy_threshold_invalid_unit_str.json", 0600);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_invalid_unit_str.json"), -1);

    chmod("./config/policy_threshold_invalid_unit_str.json", 0700);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_invalid_unit_str.json"), -13);

    chmod("./config/policy_threshold_invalid_unit_str.json", 0600);
    chown("./config/policy_threshold_invalid_unit_str.json", 1, 1);
    CU_ASSERT_EQUAL(init_mem_policy("./config/policy_threshold_invalid_unit_str.json"), -13);
}

int add_tests(void)
{
    /* add test case for memdcd_policy */
    CU_pSuite suite_memdcd_policy = CU_add_suite("memdcd_policy", NULL, NULL);
    if (suite_memdcd_policy == NULL) {
        return -1;
    }

    if (CU_ADD_TEST(suite_memdcd_policy, test_memdcd_policy) == NULL) {
        return -1;
    }

    CU_set_output_filename("memdcd");
    return 0;
}
