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
 * Description: This is a header file of the function declaration for scan function.
 ******************************************************************************/

#ifndef ETMEMD_SCAN_H
#define ETMEMD_SCAN_H

#include <fcntl.h>
#include "etmemd.h"
#include "etmemd_task.h"
#include "etmemd_scan_exp.h"
#include "etmemd_common.h"

#define VMA_SEG_CNT_MAX         6
#define VMA_PERMS_STR_LEN       5
#define VMA_ADDR_STR_LEN        17
#define PAGE_SHIFT              12
#define EPT_IDLE_BUF_MIN        ((sizeof(u_int64_t) + 2) * 2)
#define PIP_CMD_SET_HVA         (unsigned char)((PIP_CMD << 4) & 0xF0)

#define MAPS_FILE               "/maps"
#define IDLE_SCAN_FILE          "/idle_pages"

#define SMAPS_FILE              "/smaps"
#define VMFLAG_HEAD             "VmFlags"

#define IDLE_SCAN_MAGIC         0x66
#define IDLE_SCAN_ADD_FLAGS     _IOW(IDLE_SCAN_MAGIC, 0x0, unsigned int)
#define VMA_SCAN_ADD_FLAGS      _IOW(IDLE_SCAN_MAGIC, 0x2, unsigned int)
#define ALL_SCAN_FLAGS          (SCAN_AS_HUGE | SCAN_IGN_HOST | VMA_SCAN_FLAG)

enum page_idle_type {
    PTE_ACCESS = 0,     /* 4k page */
    PMD_ACCESS,         /* 2M page */
    PUD_PRESENT,        /* 1G page */
    MAX_ACCESS = PUD_PRESENT,
    PTE_DIRTY,
    PMD_DIRTY,
    PTE_IDLE,
    PMD_IDLE,
    PMD_IDLE_PTES,      /* all PTE idle */
    PTE_HOLE,
    PMD_HOLE,
    PIP_CMD,            /* 10 0xa */
};

enum access_type_weight {
    IDLE_TYPE_WEIGHT = 0,
    READ_TYPE_WEIGHT = 1,
    WRITE_TYPE_WEIGHT = 3,
    MAX_ACCESS_WEIGHT = WRITE_TYPE_WEIGHT,
};

struct walk_address {
    uint64_t walk_start;                /* walk address start */
    uint64_t walk_end;                  /* walk address end */
    uint64_t last_walk_end;             /* last walk address end */
};

/* the caller need to judge value returned by etmemd_do_scan(), NULL means fail. */
struct page_refs *etmemd_do_scan(const struct task_pid *tpid, const struct task *tk);

/* free vma list struct */
void free_vmas(struct vmas *vmas);

struct page_refs **walk_vmas(int fd, struct walk_address *walk_address, struct page_refs **pf, unsigned long *use_rss);
int get_page_refs(const struct vmas *vmas, const char *pid, struct page_refs **page_refs,
                  unsigned long *use_rss, struct ioctl_para *ioctl_para);

int split_vmflags(char ***vmflags_array, char *vmflags);
struct vmas *get_vmas_with_flags(const char *pid, char **vmflags_array, int vmflags_num, bool is_anon_only);
struct vmas *get_vmas(const char *pid);

void clean_page_refs_unexpected(void *arg);
void clean_memory_grade_unexpected(void *arg);

void clean_page_sort_unexpected(void *arg);
struct page_sort *alloc_page_sort(const struct task_pid *tk_pid);
struct page_sort *sort_page_refs(struct page_refs **page_refs, const struct task_pid *tk_pid);

struct page_refs *add_page_refs_into_memory_grade(struct page_refs *page_refs, struct page_refs **list);
int init_g_page_size(void);
int page_type_to_size(enum page_type type);
#endif
