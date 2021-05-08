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
 * Description: Etmemd migration API.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "securec.h"
#include "etmemd.h"
#include "etmemd_migrate.h"
#include "etmemd_common.h"
#include "etmemd_log.h"

static char *get_swap_string(struct page_refs **page_refs, int batchsize)
{
    char *swap_str = NULL;
    char temp_str[SWAP_ADDR_LEN] = {0};
    size_t swap_str_len;
    int count = 0;

    swap_str_len = batchsize * SWAP_ADDR_LEN;
    swap_str = (char *)calloc(swap_str_len, sizeof(char));
    if (swap_str == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for swap string to write fail\n");
        return NULL;
    }

    while (*page_refs != NULL) {
        if (count >= batchsize) {
            break;
        }

        if (snprintf_s(temp_str, SWAP_ADDR_LEN, SWAP_ADDR_LEN - 1,
                       "0x%lx\n", (*page_refs)->addr) <= 0) {
            etmemd_log(ETMEMD_LOG_WARN, "snprintf addr fail 0x%lx", (*page_refs)->addr);
            break;
        }

        if (strcat_s(swap_str, swap_str_len, temp_str) != EOK) {
            etmemd_log(ETMEMD_LOG_WARN, "strcat addr fail 0x%lx", (*page_refs)->addr);
            break;
        }

        count++;
        *page_refs = (*page_refs)->next;
    }

    return swap_str;
}

static int etmemd_migrate_mem(const char *pid, const char *grade_path, struct page_refs *page_refs_list)
{
    FILE *fp = NULL;
    char *swap_str = NULL;
    struct page_refs *page_refs = page_refs_list;

    if (page_refs_list == NULL) {
        return 0;
    }

    fp = etmemd_get_proc_file(pid, grade_path, 0, "r+");
    if (fp == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "cannot open %s for pid %s\n", grade_path, pid);
        return -1;
    }

    while (page_refs != NULL) {
        /* SWAP_LIMIT is the max size of batch that write to swap procfs once */
        swap_str = get_swap_string(&page_refs, SWAP_LIMIT);
        if (swap_str == NULL) {
            etmemd_log(ETMEMD_LOG_WARN, "get swap string fail once\n");
            continue;
        }

        if (fputs(swap_str, fp) == EOF) {
            etmemd_log(ETMEMD_LOG_DEBUG, "migrate failed for pid %s, check if etmem_swap.ko installed\n", pid);
            free(swap_str);
            fclose(fp);
            return -1;
        }
        free(swap_str);
        swap_str = NULL;
    }

    fclose(fp);
    return 0;
}


int etmemd_grade_migrate(const char *pid, const struct memory_grade *memory_grade)
{
    int ret = -1;
    /*
    * Strategies will be the hot and cold condition after classification,
    * we only operate with the cold ones.
    * */
    if (etmemd_migrate_mem(pid, COLD_PAGE, memory_grade->cold_pages) != 0) {
        return ret;
    }

    ret = 0;
    return ret;
}

