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
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "securec.h"
#include "etmemd.h"
#include "etmemd_migrate.h"
#include "etmemd_project.h"
#include "etmemd_common.h"
#include "etmemd_slide.h"
#include "etmemd_log.h"

#define RECLAIM_SWAPCACHE_MAGIC         0x77
#define RECLAIM_SWAPCACHE_ON            _IOW(RECLAIM_SWAPCACHE_MAGIC, 0x1, unsigned int)
#define SET_SWAPCACHE_WMARK             _IOW(RECLAIM_SWAPCACHE_MAGIC, 0x2, unsigned int)

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

    fp = etmemd_get_proc_file(pid, grade_path, "r+");
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

static bool check_should_reclaim_swapcache(const struct task_pid *tk_pid)
{
    struct project *proj = tk_pid->tk->eng->proj;
    unsigned long mem_total;
    unsigned long swapcache_total;
    int ret;

    if (proj->swapcache_high_wmark == -1 || proj->swapcache_low_wmark == -1) {
        return false;
    }

    ret = get_mem_from_proc_file(NULL, PROC_MEMINFO, &mem_total, "MemTotal");
    if (ret != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get memtotal fail\n");
        return false;
    }

    ret = get_mem_from_proc_file(NULL, PROC_MEMINFO, &swapcache_total, "SwapCached");
    if (ret != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get swapcache_total fail\n");
        return false;
    }

    if (swapcache_total == 0 ||
        (mem_total / swapcache_total) >= (unsigned long)(MAX_SWAPCACHE_WMARK_VALUE / proj->swapcache_high_wmark)) {
        return false;
    }

    return true;
}

static int set_swapcache_wmark(const struct task_pid *tk_pid, const char *pid_str)
{
    int swapcache_wmark;
    struct project *proj = tk_pid->tk->eng->proj;
    FILE *fp = NULL;
    struct ioctl_para ioctl_para = {
        .ioctl_cmd = SET_SWAPCACHE_WMARK,
    };

    swapcache_wmark = (proj->swapcache_low_wmark & 0x00ff) | (proj->swapcache_high_wmark << 8 & 0xff00);
    ioctl_para.ioctl_parameter = swapcache_wmark;

    fp = etmemd_get_proc_file(pid_str, COLD_PAGE, "r+");
    if (fp == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get proc file %s failed.\n", COLD_PAGE);
        return -1;
    }

    if (etmemd_send_ioctl_cmd(fp, &ioctl_para) != 0) {
        fclose(fp);
        etmemd_log(ETMEMD_LOG_ERR, "set_swapcache_wmark for pid %u fail\n", tk_pid->pid);
        return -1;
    }

    fclose(fp);
    return 0;
}

int etmemd_reclaim_swapcache(const struct task_pid *tk_pid)
{
    char pid_str[PID_STR_MAX_LEN] = {0};
    FILE *fp = NULL;
    struct ioctl_para ioctl_para = {
        .ioctl_cmd = RECLAIM_SWAPCACHE_ON,
        .ioctl_parameter = 0,
    };

    if (tk_pid == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "tk_pid is null.\n");
        return -1;
    }

    if (!check_should_reclaim_swapcache(tk_pid)) {
        return 0;
    }

    if (snprintf_s(pid_str, PID_STR_MAX_LEN, PID_STR_MAX_LEN - 1, "%u", tk_pid->pid) <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf pid fail %u", tk_pid->pid);
        return -1;
    }

    if (!tk_pid->tk->eng->proj->wmark_set) {
        if (set_swapcache_wmark(tk_pid, pid_str) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "set_swapcache_wmark for pid %u fail\n", tk_pid->pid);
            return -1;
        }
        tk_pid->tk->eng->proj->wmark_set = true;
    }

    fp = etmemd_get_proc_file(pid_str, COLD_PAGE, "r+");
    if (fp == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get proc file %s fail.\n", COLD_PAGE);
        return -1;
    }

    if (etmemd_send_ioctl_cmd(fp, &ioctl_para) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "etmemd_reclaim_swapcache fail\n");
        fclose(fp);
        return -1;
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

unsigned long check_should_migrate(const struct task_pid *tk_pid)
{
    int ret = -1;
    unsigned long vm_rss;
    unsigned long vm_swap;
    unsigned long vm_cmp;
    unsigned long need_to_swap_page_num;
    char pid_str[PID_STR_MAX_LEN] = {0};
    unsigned long pagesize;
    struct slide_params *slide_params = NULL;

    if (snprintf_s(pid_str, PID_STR_MAX_LEN, PID_STR_MAX_LEN - 1, "%u", tk_pid->pid) <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf pid fail %u", tk_pid->pid);
        return 0;
    }

    ret = get_mem_from_proc_file(pid_str, STATUS_FILE, &vm_rss, VMRSS);
    if (ret != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get vmrss %s fail", pid_str);
        return 0;
    }

    ret = get_mem_from_proc_file(pid_str, STATUS_FILE, &vm_swap, VMSWAP);
    if (ret != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get swapout %s fail", pid_str);
        return 0;
    }

    slide_params = (struct slide_params *)tk_pid->tk->params;
    if (slide_params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "slide params is null");
        return 0;
    }

    vm_cmp = (vm_rss + vm_swap) / 100 * slide_params->dram_percent;
    if (vm_cmp > vm_rss) {
        etmemd_log(ETMEMD_LOG_DEBUG, "migrate too much, stop migrate this time\n");
        return 0;
    }

    pagesize = get_pagesize();
    need_to_swap_page_num = KB_TO_BYTE(vm_rss - vm_cmp) / pagesize;

    return need_to_swap_page_num;
}
