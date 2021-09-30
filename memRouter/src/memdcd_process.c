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
#include <stdio.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "memdcd_log.h"
#include "memdcd_policy.h"
#include "memdcd_migrate.h"
#include "memdcd_message.h"

#define PID_MAX_FILE "/proc/sys/kernel/pid_max"
#define PID_MAX_LEN 256

struct migrate_process {
    int pid;

    struct migrate_page_list *page_list;
    uint64_t offset;
    struct timeval timestamp;

    pthread_t worker;

    struct migrate_process *prev;
    struct migrate_process *next;
};

struct migrate_process g_process_head = {
    .prev = &g_process_head,
    .next = &g_process_head,
};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

time_t collect_page_timeout = DEFAULT_COLLECT_PAGE_TIMEOUT;

static void migrate_process_free(struct migrate_process *p)
{
    p->prev->next = p->next;
    p->next->prev = p->prev;

    if (p->page_list)
        free(p->page_list);
    free(p);
}

static void migrate_process_remove(int pid)
{
    struct migrate_process *cur;

    cur = g_process_head.next;

    memdcd_log(_LOG_DEBUG, "Remove process %d.", pid);
    while (cur != &g_process_head) {
        if (cur->pid == pid)
            break;
        cur = cur->next;
    }
    if (cur == &g_process_head) {
        memdcd_log(_LOG_DEBUG, "Failed to remove process %d: not exist.", pid);
        return;
    }

    migrate_process_free(cur);
    memdcd_log(_LOG_DEBUG, "End free process %d.", pid);
}

static struct migrate_process *migrate_process_search(int pid)
{
    struct migrate_process *cur;

    cur = g_process_head.next;
    while (cur != &g_process_head) {
        if (cur->pid == pid) {
            return cur;
        }
        cur = cur->next;
    }

    return NULL;
}

static struct migrate_process *migrate_process_add(int pid, uint64_t len)
{
    struct migrate_process *process = NULL;

    process = (struct migrate_process *)malloc(sizeof(struct migrate_process));
    if (process == NULL)
        return NULL;

    process->page_list =
        (struct migrate_page_list *)malloc(sizeof(struct migrate_page) * len + sizeof(struct migrate_page_list));
    if (process->page_list == NULL) {
        free(process);
        return NULL;
    }

    process->pid = pid;
    process->page_list->length = len;
    process->offset = 0;
    process->worker = 0;

    process->next = g_process_head.next;
    process->next->prev = process;
    g_process_head.next = process;
    process->prev = &g_process_head;

    memdcd_log(_LOG_INFO, "Add new process %d.", pid);

    return process;
}

static void migrate_process_recycle(void)
{
    static struct timeval last = {0};
    struct timeval now;
    struct migrate_process *iter = g_process_head.next, *next = NULL;
    time_t time_lag;

    if (collect_page_timeout == 0)
        return;

    gettimeofday(&now, NULL);
    if (last.tv_sec == 0 || now.tv_sec - last.tv_sec < collect_page_timeout) {
        last = now;
        return;
    }
    last = now;

    pthread_mutex_lock(&mutex);
    while (iter != &g_process_head) {
        next = iter->next;
        time_lag = now.tv_sec - iter->timestamp.tv_sec;
        if (time_lag > collect_page_timeout && !(iter->worker && pthread_tryjoin_np(iter->worker, NULL) == EBUSY)) {
            memdcd_log(_LOG_WARN, "Process exceed collect page timeout %ld: %ld.", time_lag, collect_page_timeout);
            migrate_process_free(iter);
        }
        iter = next;
    }
    pthread_mutex_unlock(&mutex);

}

static pthread_t get_active_tid(void)
{
    pthread_mutex_lock(&mutex);
    struct migrate_process *iter = g_process_head.next;
    pthread_t tid = iter->worker;
    pthread_mutex_unlock(&mutex);
    return (iter == &g_process_head) ? 0 : tid;
}

void migrate_process_exit(void)
{
    pthread_t tid;
    migrate_process_recycle();
    while ((tid = get_active_tid()) != 0) {
        pthread_join(tid, NULL);
    }
}

static struct migrate_process *migrate_process_collect_pages(int pid, const struct swap_vma_with_count *vma)
{
    uint64_t count = vma->length / sizeof(struct vma_addr_with_count);
    struct migrate_process *process = NULL;
    uint64_t i;

    migrate_process_recycle();

    pthread_mutex_lock(&mutex);
    process = migrate_process_search(pid);
    if (process != NULL) {
        if (process->worker != 0 && pthread_tryjoin_np(process->worker, NULL) == EBUSY) {
            memdcd_log(_LOG_DEBUG, "Previous send work of process %d has been doing. discard pages.", pid);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        if (vma->status == MEMDCD_SEND_START) {
            memdcd_log(_LOG_DEBUG, "Previous send work of process %d is interrupted.", pid);
            migrate_process_remove(pid);
            process = NULL;
        }
    }
    if (process == NULL) {
        if (vma->status != MEMDCD_SEND_START) {
            memdcd_log(_LOG_DEBUG, "Current send work of process %d is incomplete.", pid);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        process = migrate_process_add(pid, vma->total_length);
        if (process == NULL) {
            memdcd_log(_LOG_ERROR, "Cannot allocate space for process %d.", pid);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
    }
    pthread_mutex_unlock(&mutex);

    memdcd_log(_LOG_DEBUG, "Collect %d pages for process %d; %lu has been collected; total %lu.", count, pid,
        process->offset, process->page_list->length);

    if (process->offset + count > process->page_list->length) {
        memdcd_log(_LOG_ERROR, "Collected pages of process %d is greater than total count: %lu %lu %lu.", pid,
            process->offset, count, process->page_list->length);
        migrate_process_remove(pid);
        return NULL;
    }

    for (i = 0; i < count; i++) {
        process->page_list->pages[process->offset + i].addr = vma->vma_addrs[i].vma.start_addr;
        process->page_list->pages[process->offset + i].length = vma->vma_addrs[i].vma.vma_len;
        process->page_list->pages[process->offset + i].visit_count = vma->vma_addrs[i].count;
    }
    process->offset += count;
    gettimeofday(&process->timestamp, NULL);

    if (vma->status == MEMDCD_SEND_END && process->offset != process->page_list->length) {
        memdcd_log(_LOG_ERROR, "Count of pages of process %d is not equal to total count: %lu %lu.",
            pid, process->offset, process->page_list->length);
        migrate_process_remove(pid);
        return NULL;
    }
    if (vma->status != MEMDCD_SEND_PROCESS && process->offset == process->page_list->length) {
        memdcd_log(_LOG_INFO, "Collected %lu vmas for process %d.", process->page_list->length, pid);
        return process;
    }

    return NULL;
}

void init_collect_pages_timeout(time_t timeout)
{
    collect_page_timeout = timeout;
}

static void *memdcd_migrate(void *args)
{
    struct migrate_process *process = (struct migrate_process *)args;
    struct mem_policy *policy = NULL;
    struct migrate_page_list *pages = NULL, *pages_to_numa = NULL, *pages_to_swap = NULL;

    if (process == NULL) {
        memdcd_log(_LOG_ERROR, "Process freed.");
        return NULL;
    }

    policy = get_policy();
    if (policy == NULL) {
        memdcd_log(_LOG_ERROR, "Policy not initialized.");
        return NULL;
    }

    pages = (struct migrate_page_list *)malloc(sizeof(struct migrate_page_list) +
        sizeof(struct migrate_page) * process->page_list->length);
    if (pages == NULL)
        return NULL;
    memcpy(pages, process->page_list,
        sizeof(struct migrate_page_list) + sizeof(struct migrate_page) * process->page_list->length);

    if (policy->opt->parse(policy, process->pid, pages, &pages_to_numa, &pages_to_swap)) {
        memdcd_log(_LOG_ERROR, "Error parsing policy.");
        goto free_pages;
    }

    if (pages_to_swap->pages != NULL)
        send_to_userswap(process->pid, pages_to_swap);

    if (pages_to_numa != NULL)
        free(pages_to_numa);
    if (pages_to_swap != NULL)
        free(pages_to_swap);

free_pages:
    free(pages);

    pthread_mutex_lock(&mutex);
    migrate_process_remove(process->pid);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

static int get_pid_max(void)
{
    FILE *fp;
    char buf[PID_MAX_LEN] = {0};
    char error_str[ERROR_STR_MAX_LEN] = {0};

    fp = fopen(PID_MAX_FILE, "r");
    if (fp == NULL) {
        memdcd_log(_LOG_ERROR, "Error opening pid_max file %s. err: %s",
            PID_MAX_FILE, strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
        return 0;
    }
    if (fread(buf, sizeof(buf), 1, fp) == 0) {
        if (feof(fp) == 0) {
            memdcd_log(_LOG_ERROR, "Error reading pid_max file %s.", PID_MAX_FILE);
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return atoi(buf);
}

int migrate_process_get_pages(int pid, const struct swap_vma_with_count *vma)
{
    char error_str[ERROR_STR_MAX_LEN] = {0};
    struct migrate_process *process = NULL;
    if (pid <= 0 || pid > get_pid_max()) {
        memdcd_log(_LOG_ERROR, "Invalid input pid:%d.\n ", pid);
        return -1;
    }

    process = migrate_process_collect_pages(pid, vma);

    if (process != NULL) {
        if (pthread_create(&process->worker, NULL, memdcd_migrate, (void *)process) != 0) {
            memdcd_log(_LOG_ERROR, "Error creating pthread for process %d. err: %s",
                process->pid, strerror_r(errno, error_str, ERROR_STR_MAX_LEN));
            return -1;
        }
    } else {
        return -1;
    }
    return 0;
}

