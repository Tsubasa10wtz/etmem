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
 * Description: Memigd slide API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "securec.h"
#include "etmemd_log.h"
#include "etmemd_common.h"
#include "etmemd_engine.h"
#include "etmemd_slide.h"
#include "etmemd_scan.h"
#include "etmemd_migrate.h"
#include "etmemd_pool_adapter.h"
#include "etmemd_file.h"

static struct memory_grade *slide_policy_interface(struct page_refs **page_refs, void *params)
{
    struct slide_params *slide_params = (struct slide_params *)params;
    struct memory_grade *memory_grade = NULL;

    if (params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "cannot get params for slide\n");
        return NULL;
    }

    memory_grade = (struct memory_grade *)calloc(1, sizeof(struct memory_grade));
    if (memory_grade == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for memory grade fail\n");
        return NULL;
    }

    while (*page_refs != NULL) {
        if ((*page_refs)->count >= slide_params->t) {
            *page_refs = add_page_refs_into_memory_grade(*page_refs, &memory_grade->hot_pages);
            continue;
        }
        *page_refs = add_page_refs_into_memory_grade(*page_refs, &memory_grade->cold_pages);
    }

    return memory_grade;
}

static int slide_do_migrate(unsigned int pid, const struct memory_grade *memory_grade)
{
    int ret;
    char pid_str[PID_STR_MAX_LEN] = {0};

    if (memory_grade == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "memory grade for slide should not be NULL for pid %u\n", pid);
        return -1;
    }

    if (snprintf_s(pid_str, PID_STR_MAX_LEN, PID_STR_MAX_LEN - 1, "%u", pid) <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf pid fail %u", pid);
        return -1;
    }

    /* we swap the cold pages for temporary, and do other operations later */
    ret = etmemd_grade_migrate(pid_str, memory_grade);
    return ret;
}

static void *slide_executor(void *arg)
{
    struct task_pid *tk_pid = (struct task_pid *)arg;
    struct page_refs *page_refs = NULL;
    struct memory_grade *memory_grade = NULL;

    /* register cleanup function in case of unexpected cancellation detected,
     * and register for memory_grade first, because it needs to clean after page_refs is cleaned */
    pthread_cleanup_push(clean_memory_grade_unexpected, &memory_grade);
    pthread_cleanup_push(clean_page_refs_unexpected, &page_refs);

    page_refs = etmemd_do_scan(tk_pid, tk_pid->tk);
    if (page_refs != NULL) {
        memory_grade = slide_policy_interface(&page_refs, tk_pid->tk->params);
    }

    /* no need to use page_refs any longer.
     * pop the cleanup function with parameter 1, because the items in page_refs list will be moved
     * into the at least on list of memory_grade after polidy function called if no problems happened,
     * but mig_policy_func() may fails to move page_refs in rare cases.
     * It will do nothing if page_refs is NULL */
    pthread_cleanup_pop(1);

    if (memory_grade == NULL) {
        etmemd_log(ETMEMD_LOG_DEBUG, "pid %u memory grade is empty\n", tk_pid->pid);
        goto exit;
    }

    if (slide_do_migrate(tk_pid->pid, memory_grade) != 0) {
        etmemd_log(ETMEMD_LOG_DEBUG, "slide migrate for pid %u fail\n", tk_pid->pid);
    }

exit:
    /* clean memory_grade here */
    pthread_cleanup_pop(1);
    if (malloc_trim(0) == 0) {
        etmemd_log(ETMEMD_LOG_INFO, "malloc_trim to release memory for pid %u fail\n", tk_pid->pid);
    }

    return NULL;
}

static int fill_task_threshold(void *obj, void *val)
{
    struct slide_params *params = (struct slide_params *)obj;
    int t = parse_to_int(val);

    if (t < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "slide engine param T should not be less than 0");
        return -1;
    }

    params->t = t;
    return 0;
}

static struct config_item g_slide_task_config_items[] = {
    {"T", INT_VAL, fill_task_threshold, false},
};

static int slide_fill_task(GKeyFile *config, struct task *tk)
{
    struct slide_params *params = calloc(1, sizeof(struct slide_params));

    if (params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc slide param fail\n");
        return -1;
    }

    if (parse_file_config(config, TASK_GROUP, g_slide_task_config_items, ARRAY_SIZE(g_slide_task_config_items),
                          (void *)params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "slide fill task fail\n");
        goto free_params;
    }

    if (params->t > tk->eng->proj->loop * WRITE_TYPE_WEIGHT) {
        etmemd_log(ETMEMD_LOG_ERR, "engine param T must less than loop.\n");
        goto free_params;
    }
    tk->params = params;
    return 0;

free_params:
    free(params);
    return -1;
}

static void slide_clear_task(struct task *tk)
{
    free(tk->params);
    tk->params = NULL;
}

static int slide_start_task(struct engine *eng, struct task *tk)
{
    struct slide_params *params = tk->params;

    params->executor = malloc(sizeof(struct task_executor));
    if (params->executor == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "slide alloc memory for task_executor fail\n");
        return -1;
    }

    params->executor->tk = tk;
    params->executor->func = slide_executor;
    if (start_threadpool_work(params->executor) != 0) {
        free(params->executor);
        params->executor = NULL;
        etmemd_log(ETMEMD_LOG_ERR, "slide start task executor fail\n");
        return -1;
    }

    return 0;
}

static void slide_stop_task(struct engine *eng, struct task *tk)
{
    struct slide_params *params = tk->params;

    stop_and_delete_threadpool_work(tk);
    free(params->executor);
    params->executor = NULL;
}

struct engine_ops g_slide_eng_ops = {
    .fill_eng_params = NULL,
    .clear_eng_params = NULL,
    .fill_task_params = slide_fill_task,
    .clear_task_params = slide_clear_task,
    .start_task = slide_start_task,
    .stop_task = slide_stop_task,
    .alloc_pid_params = NULL,
    .free_pid_params = NULL,
    .eng_mgt_func = NULL,
};

int fill_engine_type_slide(struct engine *eng)
{
    eng->ops = &g_slide_eng_ops;
    eng->engine_type = SLIDE_ENGINE;
    eng->name = "slide";
    return 0;
}
