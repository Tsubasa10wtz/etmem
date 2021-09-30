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
 * Description: function of threshold policy
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <numaif.h>
#include <numa.h>
#include <json-c/json.h>
#include <json-c/json_util.h>
#include <json-c/json_object.h>

#include "memdcd_log.h"
#include "memdcd_policy.h"
#include "memdcd_process.h"
#include "memdcd_policy_threshold.h"

struct threshold_policy {
    uint64_t threshold;
};
 
int threshold_policy_init(struct mem_policy *policy, const char *path);
int threshold_policy_parse(const struct mem_policy *policy, int pid, struct migrate_page_list *page_list,
    struct migrate_page_list **pages_to_numa, struct migrate_page_list **pages_to_swap);
int threshold_policy_destroy(struct mem_policy *policy);

struct memdcd_policy_opt threshold_policy_opt = {
    .init = threshold_policy_init,
    .parse = threshold_policy_parse,
    .destroy = threshold_policy_destroy,
};

int threshold_policy_init(struct mem_policy *policy, const char *path)
{
    int ret = 0;
    json_object *root = NULL, *obj_policy = NULL, *unit = NULL, *threshold = NULL;
    int th_val = -1;
    const char *str = NULL;
    struct threshold_policy *t = (struct threshold_policy *)malloc(sizeof(struct threshold_policy));
    if (t == NULL)
        return -1;

    root = json_object_from_file(path);
    if (root == NULL) {
        ret = -1;
        goto err_out;
    }

    obj_policy = json_object_object_get(root, "policy");
    if (obj_policy == NULL) {
        ret = -1;
        goto err_out;
    }

    unit = json_object_object_get(obj_policy, "unit");
    if (unit == NULL) {
        ret = -1;
        goto err_out;
    }

    threshold = json_object_object_get(obj_policy, "threshold");
    if (threshold == NULL) {
        ret = -1;
        goto err_out;
    }

    th_val = json_object_get_int(threshold);
    if (th_val == INT32_MAX || th_val < 0) {
        memdcd_log(_LOG_ERROR, "Invalid threshold value, allowed range is [0, INT_MAX).");
        ret = -1;
        goto err_out;
    }

    str = json_object_get_string(unit);
    if (str == NULL) {
        ret = -1;
        goto err_out;
    }

    memdcd_log(_LOG_INFO, "Threshold policy loaded.");
    if (strcmp("B", str) == 0) {
        t->threshold = th_val;
    } else if (strcmp("KB", str) == 0) {
        t->threshold = (uint64_t)th_val * 1024;
    } else if (strcmp("MB", str) == 0) {
        t->threshold = (uint64_t)th_val * 1024 * 1024;
    } else if (strcmp("GB", str) == 0) {
        t->threshold = (uint64_t)th_val * 1024 * 1024 * 1024;
    } else {
        memdcd_log(_LOG_ERROR, "Detected invalid threshold setting. Abort.");
        ret = -1;
        goto err_out;
    }

    policy->type = POL_TYPE_THRESHOLD;
    policy->opt = &threshold_policy_opt;
    policy->private = t;

    return 0;

err_out:
    free(t);
    return ret;
}

static int addr_cmp_by_count(const void *a, const void *b)
{
    return ((struct migrate_page *)a)->visit_count - ((struct migrate_page *)b)->visit_count;
}

int threshold_policy_parse(const struct mem_policy *policy, int pid, struct migrate_page_list *page_list,
    struct migrate_page_list **pages_to_numa, struct migrate_page_list **pages_to_swap)
{
    int ret = 0;
    uint64_t swap_count = 0;
    uint64_t sum_size = 0;
    int *status = NULL;
    uint64_t *move_addr = NULL;
    uint64_t length = page_list->length;

    (void)pages_to_numa;

    qsort(page_list->pages, length, sizeof(struct migrate_page), addr_cmp_by_count);

    status = malloc(sizeof(int) * length);
    if (status == NULL)
        return -1;

    move_addr = malloc(sizeof(uint64_t) * length);
    if (move_addr == NULL) {
        free(status);
        return -1;
    }

    for (uint64_t i = 0; i < length; i++)
        move_addr[i] = page_list->pages[i].addr;

    if (move_pages(pid, length, (void **)move_addr, NULL, status, MPOL_MF_MOVE) < 0) {
        memdcd_log(_LOG_ERROR, "Error when locate node for src_addr by move_page.");
        ret = -1;
        goto tpp_err_out;
    }

    for (int64_t i = length - 1; i >= 0; i--) {
        page_list->pages[i].numanode = status[i];
        if (status[i] < 0) { // negative means not on a numa node
            memdcd_log(_LOG_DEBUG, "memdcd_migrate: Error getting current node of page %lx: %d, %ld.",
                move_addr[i], status[i], i);
            continue;
        }

        /*judge if Integer Overflow happen*/
        if (sum_size + page_list->pages[i].length < sum_size) {
            ret = -1;
            goto tpp_err_out;
        }
        sum_size += page_list->pages[i].length;
        if (sum_size < ((struct threshold_policy *)policy->private)->threshold)
            continue;

        swap_count++;
        // reuse the second half array
        page_list->pages[length - swap_count] = page_list->pages[i];
    }

    *pages_to_swap = malloc(sizeof(struct migrate_page) * swap_count + sizeof(struct migrate_page_list));
    if (*pages_to_swap == NULL) {
        memdcd_log(_LOG_ERROR, "memdcd_migrate: error allocating space for pages_to_swap.");
        ret = -1;
        goto tpp_err_out;
    }

    memcpy((*pages_to_swap)->pages, page_list->pages + length - swap_count, sizeof(struct migrate_page) * swap_count);
    (*pages_to_swap)->length = swap_count;

tpp_err_out:
    free(status);
    free(move_addr);
    return ret;
}

int threshold_policy_destroy(struct mem_policy *policy)
{
    if (policy == NULL) {
        return -1;
    }
    if (policy->private == NULL) {
        return -1;
    }
    free(policy->private);
    policy->private = NULL;
    return 0;
}

struct memdcd_policy_opt *get_threshold_policy(void)
{
    return &threshold_policy_opt;
}
