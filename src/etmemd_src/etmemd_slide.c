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

static int fill_slide_param_t(struct engine *eng, const char *val)
{
    int value;
    struct slide_params *s_param = (struct slide_params *)eng->params;
    struct task *tk = (struct task *)eng->task;

    if (get_int_value(val, &value) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid engine param T value.\n");
        return -1;
    }

    /* the max value of T watermark is weight of write multipled by time of loop */
    if (value < 1 || value > tk->proj->loop * WRITE_TYPE_WEIGHT) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid engine param T value, must between 1 and loop.\n");
        return -1;
    }

    s_param->t = value;

    return 0;
}

static int parse_slide_params(struct engine *eng, FILE *file)
{
    char key[KEY_VALUE_MAX_LEN] = {};
    char value[KEY_VALUE_MAX_LEN] = {};
    char *get_line = NULL;

    get_line = skip_blank_line(file);
    if (get_line == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "lack of config for slide engine privately\n");
        return -1;
    }

    if (get_keyword_and_value(get_line, key, value) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get engine param keyword and value fail\n");
        return -1;
    }

    if (strcmp(key, "T") != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid slide parameter keyword, must be T\n");
        return -1;
    }

    if (fill_slide_param_t(eng, value) != 0) {
        return -1;
    }

    return 0;
}

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

static void *slide_exector(void *arg)
{
    struct task_pid *tk_pid = (struct task_pid *)arg;
    struct page_refs *page_refs = NULL;
    struct memory_grade *memory_grade = NULL;
    struct engine *eng = NULL;

    eng = tk_pid->tk->eng;
    if (eng == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "pid %u engine is null!\n", tk_pid->pid);
        return NULL;
    }

    if (eng->adp == NULL || eng->adp->do_scan == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "pid %u scan function is not registered!\n", tk_pid->pid);
        return NULL;
    }

    /* register cleanup function in case of unexpected cancellation detected,
     * and register for memory_grade first, because it needs to clean after page_refs is cleaned */
    pthread_cleanup_push(clean_memory_grade_unexpected, &memory_grade);
    pthread_cleanup_push(clean_page_refs_unexpected, &page_refs);

    page_refs = eng->adp->do_scan(tk_pid, tk_pid->tk);
    if (page_refs != NULL) {
        memory_grade = eng->mig_policy_func(&page_refs, eng->params);
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

    if (eng->adp->do_migrate == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "pid %u migrate function is not registered!\n", tk_pid->pid);
        goto exit;
    }

    if (eng->adp->do_migrate(tk_pid->pid, memory_grade) != 0) {
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

static int alloc_slide_params(struct task_pid **tk_pid)
{
    (*tk_pid)->params = NULL;
    return 0;
}

int fill_engine_type_slide(struct engine *eng)
{
    struct slide_params *s_param = NULL;
    struct task *tk = (struct task *)eng->task;

    s_param = (struct slide_params *)calloc(1, sizeof(struct slide_params));
    if (s_param == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc slide engine params fail\n");
        return -1;
    }

    eng->adp = (struct adapter *)calloc(1, sizeof(struct adapter));
    if (eng->adp == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc adapter for task %s engine fail\n", tk->value);
        free(s_param);
        return -1;
    }

    eng->params = (void *)s_param;
    eng->engine_type = SLIDE_ENGINE;
    eng->parse_param_conf = parse_slide_params;
    eng->mig_policy_func = slide_policy_interface;
    eng->adp->do_scan = etmemd_do_scan;
    eng->adp->do_migrate = slide_do_migrate;
    eng->alloc_params = alloc_slide_params;
    tk->start_etmem = start_threadpool_work;
    tk->stop_etmem = stop_and_delete_threadpool_work;
    tk->delete_etmem = stop_and_delete_threadpool_work;
    tk->workflow_engine = slide_exector;

    return 0;
}
