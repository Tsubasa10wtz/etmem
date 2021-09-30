/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * etmem/memRouter licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: liruilin
 * Create: 2021-02-26
 * Description: process received pages
 ******************************************************************************/
#ifndef MEMDCD_PROCESS_H
#define MEMDCD_PROCESS_H

#include <time.h>
#include "memdcd_message.h"

#define DEFAULT_COLLECT_PAGE_TIMEOUT 3

struct migrate_page {
    uint64_t addr;
    uint64_t length;
    int visit_count;

    int numanode;
};

struct migrate_page_list {
    uint64_t length;
    struct migrate_page pages[];
};

void init_collect_pages_timeout(time_t timeout);
int migrate_process_get_pages(int pid, const struct swap_vma_with_count *vma);
void migrate_process_exit(void);

#endif /* MEMDCD_MIGRATE_H */

