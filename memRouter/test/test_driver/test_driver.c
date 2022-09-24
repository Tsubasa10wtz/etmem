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
 * Description: main function for Cunit test
 * **************************************************************************** */

#include <stdlib.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

int add_tests(void);

typedef enum {
    CUNIT_SCREEN = 0,
    CUNIT_XMLFILE,
    CUNIT_CONSOLE
} CU_RunMode;

// param1: CUnitMode < 0, 1, 2 >
int main(int argc, char **argv)
{
    CU_pSuite suite;
    unsigned int num_failures;
    CU_RunMode CUnitMode = CUNIT_SCREEN;

    if (argc > 1) {
        CUnitMode = atoi(argv[1]);
    }

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    if (add_tests() != 0) {
        goto ERROR;
    }

    switch (CUnitMode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_automated_run_tests();
            break;
        case CUNIT_CONSOLE:
            CU_console_run_tests();
            break;
        default:
            printf("not suport cunit mode, only suport: 0 or 1 or 2\n");
            goto ERROR;
    }

    num_failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    return num_failures;

ERROR:
    CU_cleanup_registry();
    return CU_get_error();
}
