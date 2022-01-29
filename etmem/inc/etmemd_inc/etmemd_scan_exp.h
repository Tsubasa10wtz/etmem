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
 * Author: shikemeng
 * Create: 2021-04-30
 * Description: This is a header file of the function declaration for export scan function.
 ******************************************************************************/

#ifndef ETMEMD_SCAN_EXP_H
#define ETMEMD_SCAN_EXP_H

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>

#define VMA_PATH_STR_LEN        256
#define VMA_MAJOR_MINOR_LEN     8

#define SCAN_AS_HUGE            0100000000      /* treat normal vm page as vm hugepage */
#define SCAN_IGN_HOST           0200000000      /* ignore host access when scan vm */
#define VMA_SCAN_FLAG           0x1000          /* scan the specifics vma with flag */

enum {
    VMA_STAT_READ = 0,
    VMA_STAT_WRITE,
    VMA_STAT_EXEC,
    VMA_STAT_MAY_SHARE,
    VMA_STAT_INIT,
};

/*
 * vma struct
 * */
struct vma {
    uint64_t start;                     /* address start */
    uint64_t end;                       /* address end */
    bool stat[VMA_STAT_INIT];           /* vm area permissions */
    uint64_t offset;                    /* vm area offset */
    uint64_t inode;                     /* vm area inode */
    char path[VMA_PATH_STR_LEN];        /* path name */
    char major[VMA_MAJOR_MINOR_LEN];    /* device number major part */
    char minor[VMA_MAJOR_MINOR_LEN];    /* device number minor part */

    struct vma *next;                   /* point to next vma */
};

/*
 * vmas struct
 * */
struct vmas {
    uint64_t vma_cnt;           /* number of vm area */

    struct vma *vma_list;       /* vm area list */
};

int etmemd_scan_init(void);
void etmemd_scan_exit(void);

struct vmas *etmemd_get_vmas(const char *pid, char **vmflags_array, int vmflags_num, bool is_anon_only);
void etmemd_free_vmas(struct vmas *vmas);

int etmemd_get_page_refs(const struct vmas *vmas, const char *pid, struct page_refs **page_refs, int flags);
void etmemd_free_page_refs(struct page_refs *page_refs);

#endif
