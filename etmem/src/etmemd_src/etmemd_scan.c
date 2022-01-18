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
 * Description: Etmemd scan API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <glib.h>

#include "etmemd.h"
#include "etmemd_scan.h"
#include "etmemd_project.h"
#include "etmemd_engine.h"
#include "etmemd_common.h"
#include "etmemd_slide.h"
#include "etmemd_log.h"
#include "securec.h"

#define HEXADECIMAL_RADIX 16
#define PMD_IDLE_PTES_PARAMETER 512
#define VMFLAG_MAX_NUM 30
#define VMFLAG_VALID_LEN 2

static bool g_exp_scan_inited = false;

static const enum page_type g_page_type_by_idle_kind[] = {
    PTE_TYPE,
    PMD_TYPE,
    PUD_TYPE,
    PTE_TYPE,
    PMD_TYPE,
    PTE_TYPE,
    PMD_TYPE,
    PMD_TYPE,
    PTE_TYPE,
    PMD_TYPE,
    PAGE_TYPE_INVAL,
};

static uint64_t g_page_size[PAGE_TYPE_INVAL];

int page_type_to_size(enum page_type type)
{
    return g_page_size[type];
}

static unsigned int get_page_shift(long pagesize)
{
    unsigned int page_shift = 0;
    pagesize = pagesize >> 1;
    while (pagesize != 0) {
        page_shift++;
        pagesize = pagesize >> 1;
    }

    return page_shift;
}

int init_g_page_size(void)
{
    unsigned int page_shift;
    long pagesize;

    pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "get pagesize fail, error: %d\n", errno);
        return -1;
    }

    /* In the x86 architecture, the pagesize is 4kB. In the arm64 architecture,
     * the pagesize is 4KB, 16KB, 64KB. Therefore, the pagesize in different
     * scenarios is calculated as follows: */
    page_shift = get_page_shift(pagesize);
    g_page_size[PTE_TYPE] = 1 << page_shift;                         /* PTE_SIZE */
    g_page_size[PMD_TYPE] = 1 << (((page_shift - 3) * (4 - 2)) + 3); /* PMD_SIZE = (page_shift - 3) * (4 - 2) + 3  */
    g_page_size[PUD_TYPE] = 1 << (((page_shift - 3) * (4 - 1)) + 3); /* PUD_SIZE = (page_shift - 3) * (4 - 1) + 3  */

    return 0;
}

static bool is_anonymous(const struct vma *vma)
{
    if (vma->stat[VMA_STAT_MAY_SHARE] || vma->stat[VMA_STAT_EXEC]) {
        return false;
    }

    if (vma->inode == 0 || vma->stat[VMA_STAT_WRITE]) {
        return true;
    }

    return false;
}

void free_vmas(struct vmas *vmas)
{
    struct vma *tmp = NULL;
    if (vmas == NULL) {
        return;
    }

    while (vmas->vma_list != NULL) {
        tmp = vmas->vma_list;
        vmas->vma_list = tmp->next;
        free(tmp);
        vmas->vma_cnt--;
    }

    free(vmas);
}

static bool parse_vma_seg0(struct vma *vma, const char *seg0)
{
    int ret;
    char start[VMA_ADDR_STR_LEN] = {0};
    char end[VMA_ADDR_STR_LEN] = {0};
    char *endptr = NULL;

    ret = sscanf_s(seg0, "%[0-9;a-f]-%[0-9;a-f]", start, VMA_ADDR_STR_LEN, end, VMA_ADDR_STR_LEN);
    if (ret <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "parse address of start and end of vma fail (%d)\n", ret);
        return false;
    }

    errno = 0;
    vma->start = strtoul(start, &endptr, HEXADECIMAL_RADIX);
    if (check_str_format(endptr[0])) {
        etmemd_log(ETMEMD_LOG_ERR, "get start of vma %s fail\n", seg0);
        return false;
    }
    vma->end = strtoul(end, &endptr, HEXADECIMAL_RADIX);
    if (check_str_format(endptr[0])) {
        etmemd_log(ETMEMD_LOG_ERR, "get end of vma %s fail\n", seg0);
        return false;
    }

    return true;
}

static void parse_vma_seg1(struct vma *vma, const char *seg1)
{
    vma->stat[VMA_STAT_READ] = seg1[VMA_STAT_READ] == 'r';
    vma->stat[VMA_STAT_WRITE] = seg1[VMA_STAT_WRITE] == 'w';
    vma->stat[VMA_STAT_EXEC] = seg1[VMA_STAT_EXEC] == 'x';
    vma->stat[VMA_STAT_MAY_SHARE] = seg1[VMA_STAT_MAY_SHARE] != 'p';
}

static bool parse_vma_seg2(struct vma *vma, const char *seg2)
{
    char *endptr = NULL;

    errno = 0;
    vma->offset = strtoul(seg2, &endptr, HEXADECIMAL_RADIX);
    if (check_str_format(endptr[0])) {
        etmemd_log(ETMEMD_LOG_ERR, "get offset %s of vma fail\n", seg2);
        return false;
    }

    return true;
}

static bool parse_vma_seg3(const struct vma *vma, const char *seg3)
{
    int ret;

    ret = sscanf_s(seg3, "%[0-9;a-f]:%[0-9;a-f]", vma->major, VMA_MAJOR_MINOR_LEN, vma->minor, VMA_MAJOR_MINOR_LEN);
    if (ret <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get major or minor for vma %s fail\n", seg3);
        return false;
    }

    return true;
}

static bool parse_vma_seg4(struct vma *vma, const char *seg4)
{
    char *endptr = NULL;

    errno = 0;
    vma->inode = strtoul(seg4, &endptr, DECIMAL_RADIX);
    if (check_str_format(endptr[0])) {
        etmemd_log(ETMEMD_LOG_ERR, "get inode %s for vma fail\n", seg4);
        return false;
    }

    return true;
}

static bool parse_vma_seg5(struct vma *vma, const char *seg5)
{
    if (strlen(seg5) > VMA_PATH_STR_LEN - 1) {
        etmemd_log(ETMEMD_LOG_WARN, "path is too long, do not copy path %s \n", seg5);
        return true;
    }

    if (strncpy_s(vma->path, VMA_PATH_STR_LEN, seg5, strlen(seg5)) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "get path %s from vma fail\n", seg5);
        return false;
    }

    return true;
}

static struct vma *get_vma(char *line)
{
    int i = 0;
    struct vma *vma = NULL;
    char *seg[VMA_SEG_CNT_MAX] = {0};
    char *outptr = NULL;

    vma = (struct vma *)calloc(1, sizeof(struct vma));
    if (vma == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for vma fail\n");
        return NULL;
    }

    while (i < VMA_SEG_CNT_MAX && (seg[i] = strtok_r(line, " ", &outptr)) != NULL) {
        i++;
        line = NULL;
    }

    /* parse maps line
     * seg[0] address of start and end of a vma */
    if (!parse_vma_seg0(vma, seg[0])) {
        goto exit;
    }

    /* seg[1] the stat of vma indicates that read/write/executable/mayshare */
    parse_vma_seg1(vma, seg[1]);

    /* seg[2] offset of a vma in the file */
    if (!parse_vma_seg2(vma, seg[2])) {
        goto exit;
    }

    /* seg[3] device number of the mapping file */
    if (!parse_vma_seg3(vma, seg[3])) {
        goto exit;
    }

    /* seg[4] inode of the mapping file */
    if (!parse_vma_seg4(vma, seg[4])) {
        goto exit;
    }

    /* seg[5] name of the mapping file */
    if (!parse_vma_seg5(vma, seg[5])) {
        goto exit;
    }

    return vma;
exit:
    free(vma);
    return NULL;
}

static bool is_vma_with_vmflags(FILE *fp, char *vmflags_array[], int vmflags_num)
{
    char parse_line[FILE_LINE_MAX_LEN];
    size_t len;
    int i;
    char *flags_start = NULL;

    len = strlen(VMFLAG_HEAD);
    while (fgets(parse_line, FILE_LINE_MAX_LEN - 1, fp) != NULL) {
        /* skip the line which has no match length */
        if (strlen(parse_line) <= len) {
            continue;
        }
        if (strncmp(VMFLAG_HEAD, parse_line, len) != 0) {
            continue;
        }

        flags_start = strstr(parse_line, ":");
        /* check any flag in flags is set */
        for (i = 0; i < vmflags_num; i++) {
            if (strstr(flags_start + 1, vmflags_array[i]) == NULL) {
                return false;
            }
        }
        return true;
    }

    return false;
}

static bool is_vmflags_match(FILE *fp, char *vmflags_array[], int vmflags_num)
{
    if (vmflags_num == 0) {
        return true;
    }
    return is_vma_with_vmflags(fp, vmflags_array, vmflags_num);
}

static bool is_anon_match(bool is_anon_only, struct vma *vma)
{
    if (!is_anon_only) {
        return true;
    }
    return is_anonymous(vma);
}

int split_vmflags(char ***vmflags_array, char *vmflags)
{
    char *flag = NULL;
    char *saveptr = NULL;
    char *tmp_array[VMFLAG_MAX_NUM] = {};
    int vmflags_num = 0;
    int i;

    for (flag = strtok_r(vmflags, " ", &saveptr); flag != NULL; flag = strtok_r(NULL, " ,", &saveptr)) {
        tmp_array[vmflags_num++] = flag;
        if (vmflags_num == VMFLAG_MAX_NUM) {
            break;
        }
    }

    *vmflags_array = malloc(sizeof(char *) * vmflags_num);
    if (*vmflags_array == NULL) {
        return -1;
    }

    for (i = 0; i < vmflags_num; i++) {
        (*vmflags_array)[i] = tmp_array[i];
    }

    return vmflags_num;
}

struct vmas *get_vmas_with_flags(const char *pid, char *vmflags_array[], int vmflags_num, bool is_anon_only)
{
    struct vmas *ret_vmas = NULL;
    struct vma **tmp_vma = NULL;
    FILE *fp = NULL;
    char maps_line[FILE_LINE_MAX_LEN];
    size_t len;
    char *maps_file = NULL;

    if (vmflags_num == 0) {
        maps_file = MAPS_FILE;
    } else {
        maps_file = SMAPS_FILE;
    }

    ret_vmas = (struct vmas *)calloc(1, sizeof(struct vmas));
    if (ret_vmas == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for vmas fail\n");
        return NULL;
    }

    fp = etmemd_get_proc_file(pid, maps_file, "r");
    if (fp == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "open %s file of %s fail\n", maps_file, pid);
        free(ret_vmas);
        return NULL;
    }

    tmp_vma = &(ret_vmas->vma_list);
    while (fgets(maps_line, FILE_LINE_MAX_LEN - 1, fp) != NULL) {
        len = strlen(maps_line);
        *tmp_vma = get_vma(maps_line);
        if (*tmp_vma == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "get vma in line %s fail\n", maps_line);
            free_vmas(ret_vmas);
            ret_vmas = NULL;
            break;
        }

        /* if the file path is too long, maps_line cannot read file line completely, clean invalid characters */
        while (maps_line[len - 1] != '\n') {
            if (fgets(maps_line, FILE_LINE_MAX_LEN - 1, fp) != NULL) {
                len = strlen(maps_line);
                continue;
            }
        }

        /* skip vma without vmflags */
        if (!is_vmflags_match(fp, vmflags_array, vmflags_num) || !is_anon_match(is_anon_only, *tmp_vma)) {
            free(*tmp_vma);
            *tmp_vma = NULL;
            continue;
        }

        tmp_vma = &((*tmp_vma)->next);
        ret_vmas->vma_cnt++;
    }

    fclose(fp);
    return ret_vmas;
}

struct vmas *get_vmas(const char *pid)
{
    return get_vmas_with_flags(pid, NULL, 0, true);
}

static bool is_flag_valid(char *flag)
{
    if (strstr(flag, " ") != NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "flag %s include space\n", flag);
        return false;
    }
    if (strlen(flag) != VMFLAG_VALID_LEN) {
        etmemd_log(ETMEMD_LOG_ERR, "flag %s len is not 2\n", flag);
        return false;
    }

    return true;
}

struct vmas *etmemd_get_vmas(const char *pid, char *vmflags_array[], int vmflags_num, bool is_anon_only)
{
    int i;

    if (pid == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "etmemd_get_vmas pid param is NULL\n");
        return NULL;
    }

    for (i = 0; i < vmflags_num; i++) {
        if (vmflags_array[i] == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "etmemd_get_vmas vmflags_array[%d] is NULL\n", i);
            return NULL;
        }
        if (!is_flag_valid(vmflags_array[i])) {
            etmemd_log(ETMEMD_LOG_ERR, "etmemd_get_vmas flag %s invalid\n", vmflags_array[i]);
            return NULL;
        }
    }

    return get_vmas_with_flags(pid, vmflags_array, vmflags_num, is_anon_only);
}

static u_int64_t get_address_from_buf(const unsigned char *buf, u_int64_t index)
{
    u_int64_t address;

    /* 1 is buf index offset, 56 is address offset, 0xFF00000000000000ULL is mask */
    address = (((u_int64_t)buf[index + 1] << 56) & 0xFF00000000000000ULL) +
        /* 2 is buf index offset, 48 is address offset, 0x00FF000000000000ULL is mask */
        (((u_int64_t)buf[index + 2] << 48) & 0x00FF000000000000ULL) +
        /* 3 is buf index offset, 40 is address offset, 0x0000FF0000000000ULL is mask */
        (((u_int64_t)buf[index + 3] << 40) & 0x0000FF0000000000ULL) +
        /* 4 is buf index offset, 32 is address offset, 0x000000FF00000000ULL is mask */
        (((u_int64_t)buf[index + 4] << 32) & 0x000000FF00000000ULL) +
        /* 5 is buf index offset, 24 is address offset, 0x00000000FF000000ULL is mask */
        (((u_int64_t)buf[index + 5] << 24) & 0x00000000FF000000ULL) +
        /* 6 is buf index offset, 16 is address offset, 0x0000000000FF0000ULL is mask */
        (((u_int64_t)buf[index + 6] << 16) & 0x0000000000FF0000ULL) +
        /* 7 is buf index offset, 8 is address offset, 0x000000000000FF00ULL is mask */
        (((u_int64_t)buf[index + 7] << 8) & 0x000000000000FF00ULL) +
        /* 8 is buf index offset, 0x00000000000000FFULL is mask */
        ((u_int64_t)buf[index + 8] & 0x00000000000000FFULL);

    return address;
}

static int get_page_nr_from_buf(unsigned char buf)
{
    return (buf & 0x0F);
}

static enum page_idle_type get_page_type_from_buf(unsigned char buf)
{
    /* 4 is offset, 0x0F is mask  */
    return (enum page_idle_type)((buf >> 4) & 0x0F);
}

static struct page_refs *alloc_page_refs_node(u_int64_t addr, int weight, enum page_type type)
{
    struct page_refs *pf = NULL;

    pf = (struct page_refs *)calloc(1, sizeof(struct page_refs));
    if (pf == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc for page_refs fail\n");
        return NULL;
    }

    pf->addr = addr;
    pf->count = weight;
    pf->type = type;

    return pf;
}

static struct page_refs **update_page_refs(u_int64_t addr,
                                           int weight,
                                           enum page_type type,
                                           struct page_refs **page_refs)
{
    struct page_refs *tmp_pf = NULL;

    /* if it is a new list or a new node that we need to alloc */
    if (*page_refs == NULL) {
        *page_refs = alloc_page_refs_node(addr, weight, type);
        if (*page_refs == NULL) {
            return NULL;
        }
        return &((*page_refs)->next);
    }

    /* if the address is the one that we need to update */
    if (addr == (*page_refs)->addr) {
        (*page_refs)->count += weight;
        return &((*page_refs)->next);
    }

    /* if the address is behind the currnet node, return to operate next node */
    if (addr > (*page_refs)->addr) {
        page_refs = &((*page_refs)->next);
        return update_page_refs(addr, weight, type, page_refs);
    }

    /* the address must be before the current node when code gets here, alloc a node first,
     * and then insert into the list */
    tmp_pf = alloc_page_refs_node(addr, weight, type);
    if (tmp_pf == NULL) {
        /* it is no meaning to do anything else if we cannot alloc a page_refs struct */
        return NULL;
    }

    tmp_pf->next = *page_refs;
    *page_refs = tmp_pf;

    return &((*page_refs)->next);
}

static struct page_refs **record_parse_result(u_int64_t addr, enum page_idle_type type, int nr, struct page_refs **pf)
{
    int i, weight;
    enum page_type page_size_type;

    /* ignore unaligned address when walk, because pages handled need to be aligned */
    if ((addr & (page_type_to_size(g_page_type_by_idle_kind[type]) - 1)) > 0) {
        etmemd_log(ETMEMD_LOG_WARN, "ignore address %lx which not aligned %lx for type %d\n", addr,
                   page_type_to_size(g_page_type_by_idle_kind[type]), type);
        return pf;
    }

    for (i = 0; i < nr; i++) {
        if (type >= PTE_IDLE) {
            weight = IDLE_TYPE_WEIGHT;
        } else if (type >= PTE_DIRTY) {
            weight = WRITE_TYPE_WEIGHT;
        } else {
            weight = READ_TYPE_WEIGHT;
        }

        page_size_type = g_page_type_by_idle_kind[type];
        pf = update_page_refs(addr, weight, page_size_type, pf);
        if (pf == NULL) {
            break;
        }

        addr += page_type_to_size(page_size_type);
    }

    return pf;
}

static int get_process_use_rss(int nr, enum page_idle_type type)
{
    if (type >= PTE_IDLE) {
        return 0;
    }
    return nr;
}

static struct page_refs **parse_vma_result(const unsigned char *buf, u_int64_t size,
                                           struct page_refs **pf, u_int64_t *end, unsigned long *use_rss)
{
    u_int64_t i;
    u_int64_t address = 0;
    int nr;
    enum page_idle_type type;

    for (i = 0; i < size; i++) {
        if (buf[i] == PIP_CMD_SET_HVA) {
            /* in case of that read out of buffer range */
            if (i + sizeof(u_int64_t) >= size) {
                break;
            }
            address = get_address_from_buf(buf, i);
            /* skip size of address */
            i += sizeof(u_int64_t);
            continue;
        }

        if (address == 0) {
            etmemd_log(ETMEMD_LOG_ERR, "parse address fail\n");
            return NULL;
        }

        nr = get_page_nr_from_buf(buf[i]);
        type = get_page_type_from_buf(buf[i]);
        if (use_rss != NULL) {
            *use_rss += (unsigned long)get_process_use_rss(nr, type);
        }

        /* update address if the page type is hole */
        if (type == PMD_IDLE_PTES) {
            pf = record_parse_result(address, PTE_IDLE, nr * PMD_IDLE_PTES_PARAMETER, pf);
        } else if (type < PMD_IDLE_PTES) {
            pf = record_parse_result(address, type, nr, pf);
        } else {
            address = address + (u_int64_t)nr * page_type_to_size(g_page_type_by_idle_kind[type]);
            continue;
        }

        if (pf == NULL) {
            return NULL;
        }
        address = address + (u_int64_t)nr * page_type_to_size(g_page_type_by_idle_kind[type]);
    }
    *end = address;
    return pf;
}

struct page_refs **walk_vmas(int fd,
                             struct walk_address *walk_address,
                             struct page_refs **pf,
                             unsigned long *use_rss)
{
    unsigned char *buf = NULL;
    u_int64_t size;
    ssize_t recv_size;

    /* we make the buffer size as fitable as within a vma.
     * because the size of buffer passed to kernel will be calculated again (<< (3 + PAGE_SHIFT)) */
    size = ((walk_address->walk_end - walk_address->walk_start) >> 3) / page_type_to_size(PTE_TYPE);

    /* we need to compare the size to the minimum size that kernel handled */
    size = size < EPT_IDLE_BUF_MIN ? EPT_IDLE_BUF_MIN : size;
    buf = (unsigned char *)calloc(size, sizeof(unsigned char));
    if (buf == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for vma walking fail\n");
        return NULL;
    }

    if (lseek(fd, (long)walk_address->walk_start, SEEK_SET) == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "set seek of file fail (%s)\n", strerror(errno));
        free(buf);
        return NULL;
    }

    recv_size = read(fd, buf, size);
    if (recv_size <= 0) {
        free(buf);
        return pf;
    }

    pf = parse_vma_result(buf, (u_int64_t)recv_size, pf, &(walk_address->last_walk_end), use_rss);

    free(buf);
    return pf;
}

/*
* scan the process vma to get page_refs for migrate.
* use_rss: memory that is being used by the process,
* this parameter is used only in the dynamic engine to calculate the swap-in rate.
* In other policies, NULL can be directly transmitted.
* */
int get_page_refs(const struct vmas *vmas, const char *pid, struct page_refs **page_refs,
                  unsigned long *use_rss, struct ioctl_para *ioctl_para)
{
    u_int64_t i;
    FILE *scan_fp = NULL;
    int fd = -1;
    struct vma *vma = vmas->vma_list;
    struct page_refs **tmp_page_refs = NULL;
    struct walk_address walk_address = {0, 0, 0};

    scan_fp = etmemd_get_proc_file(pid, IDLE_SCAN_FILE, "r");
    if (scan_fp == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "open %s file fail\n", IDLE_SCAN_FILE);
        return -1;
    }

    if (ioctl_para != NULL && ioctl_para->ioctl_parameter != 0
            && etmemd_send_ioctl_cmd(scan_fp, ioctl_para) != 0) {
        fclose(scan_fp);
        etmemd_log(ETMEMD_LOG_ERR, "etmemd_send_ioctl_cmd %s file for pid %s fail\n", IDLE_SCAN_FILE, pid);
        return -1;
    }

    fd = fileno(scan_fp);
    if (fd == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "fileno file fail\n");
        fclose(scan_fp);
        return -1;
    }

    tmp_page_refs = page_refs;
    for (i = 0; i < vmas->vma_cnt; i++) {
        if (walk_address.last_walk_end > vma->end) {
            vma = vma->next;
            continue;
        }

        /* meeting this branch means the end of address for last scan is between the address of
         * start and end this round, so we start from lastScanEnd address in case of scan repeatly. */
        walk_address.walk_end = vma->end;
        walk_address.walk_start = vma->start;
        if (walk_address.last_walk_end > vma->start) {
            walk_address.walk_start = walk_address.last_walk_end;
        }
        tmp_page_refs = walk_vmas(fd, &walk_address, tmp_page_refs, use_rss);
        if (tmp_page_refs == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "get end of address after last walk fail\n");
            fclose(scan_fp);
            return -1;
        }

        vma = vma->next;
    }

    fclose(scan_fp);
    return 0;
}

int etmemd_get_page_refs(const struct vmas *vmas, const char *pid, struct page_refs **page_refs, int flags)
{
    struct ioctl_para ioctl_para;
    if (!g_exp_scan_inited) {
        etmemd_log(ETMEMD_LOG_ERR, "scan module is not inited before etmemd_get_page_refs\n");
        return -1;
    }

    if (vmas == NULL || pid == NULL || page_refs == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "NULL param is found in etmemd_get_page_refs\n");
        return -1;
    }

    ioctl_para.ioctl_parameter = flags & ALL_SCAN_FLAGS;
    ioctl_para.ioctl_cmd = IDLE_SCAN_ADD_FLAGS;

    return get_page_refs(vmas, pid, page_refs, NULL, &ioctl_para);
}

void etmemd_free_page_refs(struct page_refs *pf)
{
    struct page_refs *tmp_pf = NULL;

    while (pf != NULL) {
        tmp_pf = pf;
        pf = pf->next;
        free(tmp_pf);
    }
}

struct page_refs *etmemd_do_scan(const struct task_pid *tpid, const struct task *tk)
{
    int i;
    struct vmas *vmas = NULL;
    struct page_refs *page_refs = NULL;
    int ret;
    char pid[PID_STR_MAX_LEN] = {0};
    struct ioctl_para ioctl_para = {0};

    if (tk == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "task struct is null for pid %u\n", tpid->pid);
        return NULL;
    }

    struct page_scan *page_scan = (struct page_scan *)tk->eng->proj->scan_param;

    if (snprintf_s(pid, PID_STR_MAX_LEN, PID_STR_MAX_LEN - 1, "%u", tpid->pid) <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf pid fail %u", tpid->pid);
        return NULL;
    }

    /* get vmas of target pid first. */
    vmas = get_vmas(pid);
    if (vmas == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get vmas for %s fail\n", pid);
        return NULL;
    }

    ioctl_para.ioctl_cmd = VMA_SCAN_ADD_FLAGS;
    if (tk->swap_flag != 0) {
        ioctl_para.ioctl_parameter = VMA_SCAN_FLAG;
    }

    /* loop for scanning idle_pages to get result of memory access. */
    for (i = 0; i < page_scan->loop; i++) {
        ret = get_page_refs(vmas, pid, &page_refs, NULL, &ioctl_para);
        if (ret != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "scan operation failed\n");
            /* free page_refs nodes already exist */
            etmemd_free_page_refs(page_refs);
            page_refs = NULL;
            break;
        }
        sleep((unsigned)page_scan->sleep);
    }

    free_vmas(vmas);

    return page_refs;
}

void etmemd_free_vmas(struct vmas *vmas)
{
    free_vmas(vmas);
}

void clean_page_refs_unexpected(void *arg)
{
    struct page_refs **pf = (struct page_refs **)arg;

    etmemd_free_page_refs(*pf);
    *pf = NULL;
    return;
}

void clean_memory_grade_unexpected(void *arg)
{
    struct memory_grade **mg = (struct memory_grade **)arg;

    if (*mg == NULL) {
        return;
    }

    clean_page_refs_unexpected(&((*mg)->hot_pages));
    clean_page_refs_unexpected(&((*mg)->cold_pages));
    free(*mg);
    *mg = NULL;
    return;
}

void clean_page_sort_unexpected(void *arg)
{
    struct page_sort **msg = (struct page_sort **)arg;

    if (*msg == NULL) {
        return;
    }

    for (int i = 0; i < (*msg)->loop + 1; i++) {
        clean_page_refs_unexpected(&((*msg)->page_refs_sort)[i]);
    }

    free(*msg);
    *msg = NULL;

    return;
}

struct page_sort *alloc_page_sort(const struct task_pid *tpid)
{
    struct page_sort *page_sort = NULL;
    struct page_scan *page_scan = (struct page_scan *)tpid->tk->eng->proj->scan_param;

    page_sort = (struct page_sort *)calloc(1, sizeof(struct page_sort));
    if (page_sort == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "calloc page sort failed.\n");
        return NULL;
    }

    page_sort->loop = page_scan->loop;

    page_sort->page_refs_sort = (struct page_refs **)calloc((page_scan->loop + 1), sizeof(struct page_refs *));
    if (page_sort->page_refs_sort == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "calloc page refs sort failed.\n");
        free(page_sort);
        return NULL;
    }

    return page_sort; 
}

struct page_refs *add_page_refs_into_memory_grade(struct page_refs *page_refs, struct page_refs **list)
{
    struct page_refs *tmp = NULL;

    /* put the page_refs into the head of list, so we operate the secondary pointer **list */
    tmp = page_refs->next;
    page_refs->next = *list;
    *list = page_refs;

    /* return the next page_refs of the one that passed in */
    return tmp;
}

int etmemd_scan_init(void)
{
    if (g_exp_scan_inited) {
        etmemd_log(ETMEMD_LOG_ERR, "scan module already inited\n");
        return -1;
    }

    if (init_g_page_size() == -1) {
        return -1;
    }

    g_exp_scan_inited = true;
    return 0;
}

void etmemd_scan_exit(void)
{
    g_exp_scan_inited = false;
}

/* Move the colder pages by sorting page refs.
 * Use original page_refs if dram_percent is not set.
 * But, use the sorting result of page_refs, if dram_percent is set to (0, 100] */
struct page_sort *sort_page_refs(struct page_refs **page_refs, const struct task_pid *tpid)
{
    struct slide_params *slide_params = NULL;
    struct page_sort *page_sort = NULL;
    struct page_refs *page_next = NULL;

    page_sort = alloc_page_sort(tpid);
    if (page_sort == NULL)
        return NULL;

    slide_params = (struct slide_params *)tpid->tk->params;
    if (slide_params == NULL || slide_params->dram_percent == 0) {
        page_sort->page_refs = page_refs;
        return page_sort;
    }

    while (*page_refs != NULL) {
        page_next = (*page_refs)->next;
        (*page_refs)->next = (page_sort->page_refs_sort[(*page_refs)->count]);
        (page_sort->page_refs_sort[(*page_refs)->count]) = *page_refs;
        *page_refs = page_next;
    }

    return page_sort;
}
