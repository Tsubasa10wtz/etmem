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
 * Author: louhongxiang
 * Create: 2019-12-10
 * Description: This is a header file of the function declaration for migration API.
 ******************************************************************************/

#ifndef ETMEMD_MIGRATE_H
#define ETMEMD_MIGRATE_H

#include "etmemd.h"
#include "etmemd_task.h"

#define COLD_PAGE   "/swap_pages"

/* set the max count of address to swap to 200, to avoid that kernel needs to alloc more
 * than one 4K page to store the address */
#define SWAP_LIMIT      200
#define SWAP_ADDR_LEN   20

int etmemd_grade_migrate(const char* pid, const struct memory_grade *memory_grade);
int etmemd_reclaim_swapcache(const struct task_pid *tk_pid);
unsigned long check_should_migrate(const struct task_pid *tk_pid);
#endif
