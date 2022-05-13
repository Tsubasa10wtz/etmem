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
 * Create: 2021-4-19
 * Description: Etmemd cslide API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <numaif.h>
#include <numa.h>
#include <linux/limits.h>
#include <time.h>

#include "securec.h"
#include "etmemd_log.h"
#include "etmemd_common.h"
#include "etmemd_engine.h"
#include "etmemd_cslide.h"
#include "etmemd_scan.h"
#include "etmemd_migrate.h"
#include "etmemd_file.h"

#define HUGE_1M_SIZE    (1 << 20)
#define HUGE_2M_SIZE    (2 << 20)
#define BYTE_TO_KB(s)   ((s) >> 10)
#define KB_TO_BYTE(s)   ((s) << 10)
#define HUGE_2M_TO_KB(s) ((s) << 11)

#define BATCHSIZE (1 << 16)

#define factory_foreach_working_pid_params(iter, factory) \
    for ((iter) = (factory)->working_head, next_working_params(&(iter)); \
            (iter) != NULL; \
            (iter) = (iter)->next, next_working_params(&(iter)))

#define factory_foreach_pid_params(iter, factory) \
    for ((iter) = (factory)->working_head; (iter) != NULL; (iter) = (iter)->next)

struct node_mem {
    long long huge_total;
    long long huge_free;
};

struct sys_mem {
    int node_num;
    struct node_mem *node_mem;
};

struct node_page_refs {
    struct page_refs *head;
    struct page_refs *tail;
    int64_t size;
    uint32_t num;
};

struct count_page_refs {
    struct node_page_refs *node_pfs;
    int node_num;
};

struct node_pair {
    int index;
    int hot_node;
    int cold_node;
};

struct node_map {
    struct node_pair *pair;
    int total_num;
    int cur_num;
};

struct node_verifier {
    int node_num;
    int *nodes_map_count;
};

struct cslide_task_params {
    bool anon_only;
    struct {
        char *vmflags_str;
        char **vmflags_array;
        int vmflags_num;
    };
    int scan_flags;
};

struct vma_pf {
    struct vma *vma;
    struct page_refs *page_refs;
};

struct node_pages_info {
    uint32_t hot;
    uint32_t cold;
};

enum pid_param_state {
    STATE_NONE = 0,
    STATE_WORKING,
    STATE_REMOVE,
    STATE_FREE,
};

struct cslide_pid_params {
    enum pid_param_state state;
    int count;
    struct count_page_refs *count_page_refs;
    struct memory_grade *memory_grade;
    struct node_pages_info *node_pages_info;
    struct vmas *vmas;
    struct vma_pf *vma_pf;
    unsigned int pid;
    struct cslide_eng_params *eng_params;
    struct cslide_task_params *task_params;
    struct cslide_pid_params *next;
};

struct cslide_params_factory {
    pthread_mutex_t mtx;
    struct cslide_pid_params *to_add_head;
    struct cslide_pid_params *to_add_tail;
    struct cslide_pid_params *working_head;
};

struct cslide_eng_params {
    struct sys_mem mem;
    struct node_map node_map;
    int hot_threshold;
    int hot_reserve;    // in MB
    int mig_quota;      // in MB
    pthread_t worker;
    pthread_mutex_t stat_mtx;
    time_t stat_time;
    struct {
        int loop;
        int interval;
        int sleep;
    };
    struct cslide_params_factory factory;
    struct node_pages_info *host_pages_info;
    bool finish;
};

struct ctrl_cap {
    long long cap;
    long long used;
};

struct node_ctrl {
    struct ctrl_cap hot_move_cap;
    struct ctrl_cap hot_prefetch_cap;
    struct ctrl_cap cold_move_cap;
    long long cold_replaced; // cold mem in hot node replace by hot mem in cold node
    long long cold_free; // free mem in cold node
    long long free; // free mem in hot node
    long long cold; // cold mem in hot node
    long long total; // total mem in hot node
    long long quota; // move quota
    long long reserve; // reserve space can't used by cold mem
};

struct flow_ctrl {
    struct node_ctrl *node_ctrl;
    int pair_num;
    int hot_enough;
    int prefetch_enough;
    int cold_enough;
};

struct page_filter {
    void (*flow_cal_func)(struct flow_ctrl *ctrl);
    long long (*flow_move_func)(struct flow_ctrl *ctrl, long long target, int node);
    bool (*flow_enough)(struct flow_ctrl *ctrl);
    void (*filter_policy)(struct page_filter *filter, struct node_pair *pair,
            struct count_page_refs *cpf, struct memory_grade *memory_grade);
    struct flow_ctrl *ctrl;
    int count_start;
    int count_end;
    int count_step;
};

struct cslide_cmd_item {
    char *name;
    int (*func)(void *params, int fd);
};

static inline int get_node_num(void)
{
    return numa_num_configured_nodes();
}

static int init_sys_mem(struct sys_mem *mem)
{
    int node_num = get_node_num();

    if (node_num <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "node number %d is invalid \n", node_num);
        return -1;
    }

    mem->node_mem = malloc(sizeof(struct node_mem) * node_num);
    if (mem->node_mem == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc node_mem fail\n");
        return -1;
    }
    mem->node_num = node_num;
    return 0;
}

static void destroy_sys_mem(struct sys_mem *mem)
{
    mem->node_num = -1;
    free(mem->node_mem);
    mem->node_mem = NULL;
}

static int read_hugepage_num(char *path)
{
    FILE *f = NULL;
    char *line = NULL;
    size_t line_len = 0;
    int nr = -1;

    f = fopen(path, "r");
    if (f == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "open file %s failed\n", path);
        return -1;
    }
    if (getline(&line, &line_len, f) < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "read free hugepage from %s fail\n", path);
        goto close_file;
    }

    nr = strtoull(line, NULL, 0);
    free(line);
close_file:
    fclose(f);
    return nr;
}

static long long get_single_huge_mem(int node, int huge_size, char *huge_state)
{
    char path[PATH_MAX];
    int nr;

    if (sprintf_s(path, PATH_MAX, "/sys/devices/system/node/node%d/hugepages/hugepages-%dkB/%s_hugepages",
                  node, huge_size, huge_state) <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf path to get hugepage number fail\n");
        return -1;
    }

    nr = read_hugepage_num(path);
    if (nr < 0) {
        return -1;
    }

    return KB_TO_BYTE((long long)nr * (long long)huge_size);
}

static long long get_node_huge_total(int node, char *huge_state)
{
    long long size_2m;

    size_2m = get_single_huge_mem(node, BYTE_TO_KB(HUGE_2M_SIZE), huge_state);
    if (size_2m < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get 2M %s page in node %d fail\n", huge_state, node);
        return -1;
    }

    return size_2m;
}

static int get_node_huge_mem(int node, struct node_mem *mem)
{
    mem->huge_total = get_node_huge_total(node, "nr");
    if (mem->huge_total <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get total hugepages of node %d fail\n", node);
        return -1;
    }
    mem->huge_free = get_node_huge_total(node, "free");
    if (mem->huge_free < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get free hugepages of node %d fail\n", node);
        return -1;
    }

    return 0;
}

static int get_node_mem(int node, struct node_mem *mem)
{
    if (get_node_huge_mem(node, mem) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get huge memory info of node %d fail\n", node);
        return -1;
    }

    return 0;
}

static int get_sys_mem(struct sys_mem *mem)
{
    int i;

    for (i = 0; i < mem->node_num; i++) {
        if (get_node_mem(i, &(mem->node_mem[i])) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "get memory info of node %d fail\n", i);
            return -1;
        }
    }

    return 0;
}

static void init_node_page_refs(struct node_page_refs *npf)
{
    npf->head = NULL;
    npf->tail = NULL;
    npf->size = 0;
    npf->num = 0;
}

static void clean_node_page_refs(struct node_page_refs *npf)
{
    clean_page_refs_unexpected(&npf->head);
    npf->tail = NULL;
    npf->size = 0;
    npf->num = 0;
}

static void npf_add_pf(struct node_page_refs *npf, struct page_refs *page_refs)
{
    if (npf->head == NULL) {
        npf->head = page_refs;
        npf->tail = page_refs;
    } else {
        npf->tail->next = page_refs;
        npf->tail = page_refs;
    }

    npf->size += page_type_to_size(page_refs->type);
    npf->num++;
}

/* must called when all pf add to npf with npf_add_pf */
static void npf_setup_tail(struct node_page_refs *npf)
{
    if (npf->tail != NULL) {
        npf->tail->next = NULL;
    }
}

static void move_npf_to_list(struct node_page_refs *npf, struct page_refs **list, long long size)
{
    struct page_refs *t = NULL;
    struct page_refs *iter = NULL;
    struct page_refs *tmp = NULL;
    long long moved_size = 0;

    if (npf->size <= size) {
        t = npf->tail;
        moved_size = npf->size;
    } else {
        for (iter = npf->head; iter != NULL && size >= moved_size + page_type_to_size(iter->type); iter = iter->next) {
            moved_size += page_type_to_size(iter->type);
            t = iter;
        }
    }

    if (t != NULL) {
        tmp = t->next;
        t->next = *list;
        *list = npf->head;
        npf->head = tmp;
        if (tmp == NULL) {
            npf->tail = NULL;
        }
    }

    npf->size -= moved_size;
    return;
}

static int init_count_page_refs(struct count_page_refs *cpf, int node_num)
{
    int i;

    cpf->node_pfs = malloc(sizeof(struct node_page_refs) * node_num);
    if (cpf->node_pfs == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc node_page_refs fail\n");
        return -1;
    }
    for (i = 0; i < node_num; i++) {
        init_node_page_refs(&cpf->node_pfs[i]);
    }
    cpf->node_num = node_num;
    return 0;
}

static void clean_count_page_refs(struct count_page_refs *cpf)
{
    int i;

    for (i = 0; i < cpf->node_num; i++) {
        clean_node_page_refs(&cpf->node_pfs[i]);
    }
}

static void destroy_count_page_refs(struct count_page_refs *cpf)
{
    clean_count_page_refs(cpf);
    free(cpf->node_pfs);
    cpf->node_pfs = NULL;
    cpf->node_num = 0;
}

static void insert_count_pfs(struct count_page_refs *cpf, struct page_refs *pf, int *nodes, int num)
{
    struct node_page_refs *npf = NULL;
    struct page_refs *next = NULL;
    int node, count, i;

    for (i = 0; i < num; i++) {
        next = pf->next;
        node = nodes[i];
        if (node < 0 || node >= cpf->node_num) {
            etmemd_log(ETMEMD_LOG_WARN, "addr %llx with invalid node %d\n", pf->addr, node);
            pf->next = NULL;
            etmemd_free_page_refs(pf);
            pf = next;
            continue;
        }
        count = pf->count;
        npf = &cpf[count].node_pfs[node];
        npf_add_pf(npf, pf);
        pf = next;
    }
}

/* must called after all page_refs insert by insert_count_pfs */
static void setup_count_pfs_tail(struct count_page_refs *cpf, int count)
{
    int node, c;
    struct node_page_refs *npf = NULL;

    for (c = 0; c <= count; c++) {
        for (node = 0; node < cpf->node_num; node++) {
            npf = &cpf[c].node_pfs[node];
            npf_setup_tail(npf);
        }
    }
}

static int init_node_map(struct node_map *node_map, int node_num)
{
    int pair_num;

    if (node_num % 2 != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "node_num is not even\n");
        return -1;
    }
    pair_num = node_num / 2;
    node_map->pair = calloc(pair_num, sizeof(struct node_pair));
    if (node_map->pair == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc memory for node map fail\n");
        return -1;
    }
    node_map->total_num = pair_num;
    node_map->cur_num = 0;
    return 0;
}

static void destroy_node_map(struct node_map *map)
{
    free(map->pair);
    map->pair = NULL;
    map->total_num = 0;
    map->cur_num = 0;
}

static int add_node_pair(struct node_map *map, int cold_node, int hot_node)
{
    if (map->cur_num == map->total_num) {
        etmemd_log(ETMEMD_LOG_ERR, "too much pair, add pair hot %d cold %d fail\n",
                   hot_node, cold_node);
        return -1;
    }
    map->pair[map->cur_num].hot_node = hot_node;
    map->pair[map->cur_num].cold_node = cold_node;
    map->pair[map->cur_num].index = map->cur_num;
    map->cur_num++;
    return 0;
}

static int init_node_verifier(struct node_verifier *nv, int node_num)
{
    nv->nodes_map_count = calloc(node_num, sizeof(int));
    if (nv->nodes_map_count == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc memroy for nodes_map_count failed\n");
        return -1;
    }

    nv->node_num = node_num;
    return 0;
}

static void destroy_node_verifier(struct node_verifier *nv)
{
    free(nv->nodes_map_count);
}

static bool is_node_valid(struct node_verifier *nv, int node)
{
    if (node < 0 || node >= nv->node_num) {
        etmemd_log(ETMEMD_LOG_ERR, "node %d out of range\n", node);
        return false;
    }

    if (nv->nodes_map_count[node] != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "node %d remap\n", node);
        return false;
    }

    nv->nodes_map_count[node]++;
    return true;
}

static bool has_node_unmap(struct node_verifier *nv)
{
    int i;
    bool ret = false;

    for (i = 0; i < nv->node_num; i++) {
        if (nv->nodes_map_count[i] == 0) {
            etmemd_log(ETMEMD_LOG_ERR, "unmap node %d\n", i);
            ret = true;
        }
    }

    return ret;
}

static void clear_task_params(struct cslide_task_params *params)
{
    if (params->vmflags_str != NULL) {
        free(params->vmflags_str);
        params->vmflags_str = NULL;
    }
    if (params->vmflags_array != NULL) {
        free(params->vmflags_array);
        params->vmflags_array = NULL;
    }
}

static struct cslide_pid_params *alloc_pid_params(struct cslide_eng_params *eng_params)
{
    int i;
    struct cslide_pid_params *params = calloc(1, sizeof(struct cslide_pid_params));
    int count = eng_params->loop * MAX_ACCESS_WEIGHT;
    int pair_num = eng_params->node_map.cur_num;
    int node_num = eng_params->mem.node_num;

    if (params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc cslide pid params fail\n");
        return NULL;
    }
    params->count_page_refs = malloc(sizeof(struct count_page_refs) * (count + 1));
    if (params->count_page_refs == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc counted page_refs fail\n");
        goto free_params;
    }

    for (i = 0; i <= count; i++) {
        if (init_count_page_refs(&params->count_page_refs[i], node_num) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "init counted page_refs fail\n");
            goto free_count_page_refs;
        }
    }

    params->count = count;
    params->memory_grade = calloc(pair_num, sizeof(struct memory_grade));
    if (params->memory_grade == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc memory_grade fail\n");
        goto free_count_page_refs;
    }

    params->node_pages_info = calloc(node_num, sizeof(struct node_pages_info));
    if (params->node_pages_info == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc pages info fail\n");
        goto free_memory_grade;
    }
    return params;

free_memory_grade:
    free(params->memory_grade);
    params->memory_grade = NULL;
free_count_page_refs:
    for (i--; i >= 0; i--) {
        destroy_count_page_refs(&params->count_page_refs[i]);
    }
    free(params->count_page_refs);
    params->count_page_refs = NULL;
free_params:
    free(params);
    params = NULL;
    return NULL;
}

static void free_pid_params(struct cslide_pid_params *params)
{
    int count = params->count;
    int i;

    free(params->node_pages_info);
    params->node_pages_info = NULL;
    free(params->memory_grade);
    params->memory_grade = NULL;
    for (i = 0; i <= count; i++) {
        destroy_count_page_refs(&params->count_page_refs[i]);
    }
    free(params->count_page_refs);
    params->count_page_refs = NULL;
    if (params->task_params != NULL) {
        clear_task_params(params->task_params);
        free(params->task_params);
        params->task_params = NULL;
    }
    free(params);
}

static void clean_pid_param(struct cslide_pid_params *pid_params)
{
    struct cslide_eng_params *eng_params = pid_params->eng_params;
    int pair_num = eng_params->node_map.cur_num;
    int i;

    for (i = 0; i < pair_num; i++) {
        clean_page_refs_unexpected(&pid_params->memory_grade[i].hot_pages);
        clean_page_refs_unexpected(&pid_params->memory_grade[i].cold_pages);
    }
    for (i = 0; i <= pid_params->count; i++) {
        clean_count_page_refs(&pid_params->count_page_refs[i]);
    }
}

static void destroy_factory(struct cslide_params_factory *factory)
{
    pthread_mutex_destroy(&factory->mtx);
}

static int init_factory(struct cslide_params_factory *factory)
{
    return pthread_mutex_init(&factory->mtx, NULL);
}

static void factory_add_pid_params(struct cslide_params_factory *factory, struct cslide_pid_params *params)
{
    enum pid_param_state state = params->state;
    params->state = STATE_WORKING;

    if (state == STATE_NONE) {
        pthread_mutex_lock(&factory->mtx);
        params->next = factory->to_add_head;
        factory->to_add_head = params;
        if (factory->to_add_tail == NULL) {
            factory->to_add_tail = params;
        }
        pthread_mutex_unlock(&factory->mtx);
    }
}

static void factory_remove_pid_params(struct cslide_params_factory *factory, struct cslide_pid_params *params)
{
    params->state = STATE_REMOVE;
}

static void factory_free_pid_params(struct cslide_params_factory *factory, struct cslide_pid_params *params)
{
    // Pid not started i.e not used. Free it here
    if (params->state == STATE_NONE) {
        free_pid_params(params);
        return;
    }

    // Pid in use, free by cslide main when call factory_update_pid_params
    // Avoid data race
    params->state = STATE_FREE;
}

static void next_working_params(struct cslide_pid_params **params)
{
    while (*params != NULL && (*params)->state != STATE_WORKING) {
        *params = (*params)->next;
    }
}

/* add and free operations will take effect here */
static void factory_update_pid_params(struct cslide_params_factory *factory)
{
    struct cslide_pid_params **prev = NULL;
    struct cslide_pid_params *iter = NULL;
    struct cslide_pid_params *to_add_head = NULL;
    struct cslide_pid_params *to_add_tail = NULL;

    /* get new added params first */
    pthread_mutex_lock(&factory->mtx);
    to_add_head = factory->to_add_head;
    to_add_tail = factory->to_add_tail;
    factory->to_add_head = NULL;
    factory->to_add_tail = NULL;
    pthread_mutex_unlock(&factory->mtx);

    if (to_add_head != NULL) {
        to_add_tail->next = factory->working_head;
        factory->working_head = to_add_head;
    }

    /* clear the freed params */
    prev = &factory->working_head;
    for (iter = *prev; iter != NULL; iter = *prev) {
        if (iter->state != STATE_FREE) {
            prev = &(iter->next);
            continue;
        }
        *prev = iter->next;
        iter->next = NULL;
        free_pid_params(iter);
    }
}

static bool factory_working_empty(struct cslide_params_factory *factory)
{
    struct cslide_pid_params *pid_params = NULL;

    factory_foreach_pid_params(pid_params, factory) {
        if (pid_params->state == STATE_WORKING) {
            return false;
        }
    }
    return true;
}

static struct page_refs *next_vma_pf(struct cslide_pid_params *params, int *cur)
{
    int vma_pf_num = params->vmas->vma_cnt;
    struct vma_pf *vma_pf = params->vma_pf;
    struct page_refs *pf = NULL;

    while (*cur < vma_pf_num) {
        pf = vma_pf[*cur].page_refs;
        vma_pf[*cur].page_refs = NULL;
        (*cur)++;
        if (pf != NULL) {
            break;
        }
    }
    return pf;
}

static int cslide_count_node_pfs(struct cslide_pid_params *params)
{
    struct page_refs *page_refs = NULL;
    struct page_refs *last = NULL;
    unsigned int pid = params->pid;
    int batch_size = BATCHSIZE;
    void **pages = NULL;
    int *status = NULL;
    int actual_num = 0;
    int ret = 0;
    int vma_i = 0;

    if (params->vmas == NULL || params->vma_pf == NULL) {
        return 0;
    }

    status = malloc(sizeof(int) * batch_size);
    if (status == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc status fail\n");
        return -1;
    }

    pages = malloc(sizeof(void *) * batch_size);
    if (pages == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc pages fail\n");
        ret = -1;
        goto free_status;
    }

    page_refs = next_vma_pf(params, &vma_i);
    last = page_refs;
    while (page_refs != NULL) {
        pages[actual_num++] = (void *)page_refs->addr;
        if (page_refs->next == NULL) {
            page_refs->next = next_vma_pf(params, &vma_i);
        }
        if (actual_num == batch_size || page_refs->next == NULL) {
            if (move_pages(pid, actual_num, pages, NULL, status, MPOL_MF_MOVE_ALL) != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "get page refs numa node fail\n");
                clean_page_refs_unexpected(&last);
                ret = -1;
                break;
            }
            insert_count_pfs(params->count_page_refs, last, status, actual_num);
            last = page_refs->next;
            actual_num = 0;
        }
        page_refs = page_refs->next;
    }

    // this must be called before return
    setup_count_pfs_tail(params->count_page_refs, params->count);

    free(pages);
    pages = NULL;
free_status:
    free(status);
    status = NULL;
    return ret;
}

static inline bool cap_avail(struct ctrl_cap *cap)
{
    return cap->used < cap->cap;
}

/* return true if cap is still available */
static inline bool cap_cost(struct ctrl_cap *cap, long long *cost)
{
    if (*cost < cap->cap - cap->used) {
        cap->used += *cost;
        return true;
    }

    *cost = cap->cap - cap->used;
    cap->used = cap->cap;
    return false;
}

/* return true if node can move hot pages */
static bool node_cal_hot_can_move(struct node_ctrl *node_ctrl)
{
    long long can_move;

    // can_move limited by quota
    if (node_ctrl->quota < node_ctrl->free) {
        can_move = node_ctrl->quota;
    } else {
        // can_move limited by hot node free
        can_move = node_ctrl->free + (node_ctrl->quota - node_ctrl->free) / 2;
        // can_move limited by cold node free
        if (can_move > node_ctrl->free + node_ctrl->cold_free) {
            can_move = node_ctrl->free + node_ctrl->cold_free;
        }
    }

    // can_move limited by free and cold mem in hot node
    if (can_move > node_ctrl->cold + node_ctrl->free) {
        can_move = node_ctrl->cold + node_ctrl->free;
    }
    node_ctrl->hot_move_cap.cap = can_move;
    return can_move > 0;
}

static inline bool node_can_move_hot(struct node_ctrl *node_ctrl)
{
    return cap_avail(&node_ctrl->hot_move_cap);
}

static inline bool node_move_hot(struct node_ctrl *node_ctrl, long long *target)
{
    return cap_cost(&node_ctrl->hot_move_cap, target);
}

static void node_update_hot_move(struct node_ctrl *node_ctrl)
{
    long long hot_move = node_ctrl->hot_move_cap.used;

    if (hot_move > node_ctrl->free) {
        node_ctrl->cold_replaced += hot_move - node_ctrl->free;
        node_ctrl->quota -= node_ctrl->free + (hot_move - node_ctrl->free) * 2;
        node_ctrl->cold_free += node_ctrl->free;
        node_ctrl->free = 0;
    } else {
        node_ctrl->free -= hot_move;
        node_ctrl->quota -= hot_move;
        node_ctrl->cold_free += hot_move;
    }
}

/* return true if node can prefetch hot pages */
static bool node_cal_hot_can_prefetch(struct node_ctrl *node_ctrl)
{
    long long can_prefetch;

    if (node_ctrl->free <= node_ctrl->reserve) {
        can_prefetch = 0;
        goto exit;
    }

    can_prefetch = node_ctrl->free - node_ctrl->reserve;
    if (can_prefetch > node_ctrl->quota) {
        can_prefetch = node_ctrl->quota;
    }

exit:
    node_ctrl->hot_prefetch_cap.cap = can_prefetch;
    return can_prefetch > 0;
}

static inline bool node_can_prefetch_hot(struct node_ctrl *node_ctrl)
{
    return cap_avail(&node_ctrl->hot_prefetch_cap);
}

static inline bool node_prefetch_hot(struct node_ctrl *node_ctrl, long long *target)
{
    return cap_cost(&node_ctrl->hot_prefetch_cap, target);
}

static void node_update_hot_prefetch(struct node_ctrl *node_ctrl)
{
    long long hot_prefetch = node_ctrl->hot_prefetch_cap.used;

    node_ctrl->free -= hot_prefetch;
    node_ctrl->quota -= hot_prefetch;
    node_ctrl->cold_free += hot_prefetch;
}

static bool node_cal_cold_can_move(struct node_ctrl *node_ctrl)
{
    long long can_move;

    can_move = node_ctrl->quota < node_ctrl->reserve - node_ctrl->free ?
        node_ctrl->quota : node_ctrl->reserve - node_ctrl->free;
    if (can_move > node_ctrl->cold_free) {
        can_move = node_ctrl->cold_free;
    }
    if (can_move < 0) {
        can_move = 0;
    }

    node_ctrl->cold_move_cap.cap = can_move + node_ctrl->cold_replaced;
    return node_ctrl->cold_move_cap.cap > 0;
}

static inline bool node_can_move_cold(struct node_ctrl *node_ctrl)
{
    return cap_avail(&node_ctrl->cold_move_cap);
}

/* return true if still can move cold pages */
static inline bool node_move_cold(struct node_ctrl *node_ctrl, long long *target)
{
    return cap_cost(&node_ctrl->cold_move_cap, target);
}

static int init_flow_ctrl(struct flow_ctrl *ctrl, struct cslide_eng_params *eng_params)
{

    long long quota = (long long)eng_params->mig_quota * HUGE_1M_SIZE;
    long long reserve = (long long)eng_params->hot_reserve * HUGE_1M_SIZE;
    struct sys_mem *sys_mem = &eng_params->mem;
    struct node_map *node_map = &eng_params->node_map;
    struct node_pair *pair = NULL;
    struct node_ctrl *tmp = NULL;
    int i;

    ctrl->node_ctrl = calloc(node_map->cur_num, sizeof(struct node_ctrl));
    if (ctrl->node_ctrl == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc memory for node_ctrl fail\n");
        return -1;
    }

    ctrl->pair_num = node_map->cur_num;
    ctrl->hot_enough = 0;
    ctrl->cold_enough = 0;
    ctrl->prefetch_enough = 0;
    for (i = 0; i < ctrl->pair_num; i++) {
        pair = &node_map->pair[i];
        tmp = &ctrl->node_ctrl[i];
        tmp->cold_free = sys_mem->node_mem[pair->cold_node].huge_free;
        tmp->free = sys_mem->node_mem[pair->hot_node].huge_free;
        tmp->cold = KB_TO_BYTE((unsigned long long)eng_params->host_pages_info[pair->hot_node].cold);
        tmp->total = sys_mem->node_mem[pair->hot_node].huge_total;
        tmp->quota = quota;
        tmp->reserve = reserve;
    }
    return 0;
}

static void flow_cal_hot_can_move(struct flow_ctrl *ctrl)
{
    int i;

    for (i = 0; i < ctrl->pair_num; i++) {
        if (!node_cal_hot_can_move(&ctrl->node_ctrl[i])) {
            ctrl->hot_enough++;
        }
    }
}

static inline bool is_hot_enough(struct flow_ctrl *ctrl)
{
    return ctrl->hot_enough == ctrl->pair_num;
}

static long long ctrl_hot_move(struct flow_ctrl *ctrl, long long target, int node)
{
    struct node_ctrl *node_ctrl = &ctrl->node_ctrl[node];

    if (!node_can_move_hot(node_ctrl)) {
        return 0;
    }

    if (!node_move_hot(node_ctrl, &target)) {
        ctrl->hot_enough++;
    }

    return target;
}

static void flow_cal_hot_can_prefetch(struct flow_ctrl *ctrl)
{
    int i;

    for (i = 0; i < ctrl->pair_num; i++) {
        node_update_hot_move(&ctrl->node_ctrl[i]);
        if (!node_cal_hot_can_prefetch(&ctrl->node_ctrl[i])) {
            ctrl->prefetch_enough++;
        }
    }
}

static inline bool is_prefetch_enough(struct flow_ctrl *ctrl)
{
    return ctrl->prefetch_enough == ctrl->pair_num;
}

static long long ctrl_prefetch_hot(struct flow_ctrl *ctrl, long long target, int node)
{
    struct node_ctrl *node_ctrl = &ctrl->node_ctrl[node];

    if (!node_can_prefetch_hot(node_ctrl)) {
        return 0;
    }

    if (!node_prefetch_hot(node_ctrl, &target)) {
        ctrl->prefetch_enough++;
    }

    return target;
}

static void flow_cal_cold_can_move(struct flow_ctrl *ctrl)
{
    int i;

    for (i = 0; i < ctrl->pair_num; i++) {
        node_update_hot_prefetch(&ctrl->node_ctrl[i]);
        if (!node_cal_cold_can_move(&ctrl->node_ctrl[i])) {
            ctrl->cold_enough++;
        }
    }
}

static inline bool is_cold_enough(struct flow_ctrl *ctrl)
{
    return ctrl->cold_enough == ctrl->pair_num;
}

static long long ctrl_cold_move(struct flow_ctrl *ctrl, long long target, int node)
{
    struct node_ctrl *node_ctrl = &ctrl->node_ctrl[node];

    if (!node_can_move_cold(node_ctrl)) {
        return 0;
    }

    if (!node_move_cold(node_ctrl, &target)) {
        ctrl->cold_enough++;
    }

    return target;
}

static void destroy_flow_ctrl(struct flow_ctrl *ctrl)
{
    free(ctrl->node_ctrl);
    ctrl->node_ctrl = NULL;
}

static void do_filter(struct page_filter *filter, struct cslide_eng_params *eng_params)
{
    struct cslide_pid_params *params = NULL;
    struct count_page_refs *cpf = NULL;
    struct memory_grade *memory_grade = NULL;
    struct node_pair *pair = NULL;
    int pair_num = eng_params->node_map.cur_num;
    int i, j;

    filter->flow_cal_func(filter->ctrl);
    if (filter->flow_enough(filter->ctrl)) {
        return;
    }

    for (i = filter->count_start; i != filter->count_end; i += filter->count_step) {
        factory_foreach_working_pid_params(params, &eng_params->factory) {
            cpf = &params->count_page_refs[i];
            for (j = 0; j < pair_num; j++) {
                pair = &eng_params->node_map.pair[j];
                memory_grade = &params->memory_grade[j];
                filter->filter_policy(filter, pair, cpf, memory_grade);
                if (filter->flow_enough(filter->ctrl)) {
                    return;
                }
            }
        }
    }
}

static void to_hot_policy(struct page_filter *filter, struct node_pair *pair,
        struct count_page_refs *cpf, struct memory_grade *memory_grade)
{
    long long can_move;
    struct node_page_refs *npf = &cpf->node_pfs[pair->cold_node];

    can_move = filter->flow_move_func(filter->ctrl, npf->size, pair->index);
    move_npf_to_list(npf, &memory_grade->hot_pages, can_move);
}

static void to_cold_policy(struct page_filter *filter, struct node_pair *pair,
        struct count_page_refs *cpf, struct memory_grade *memory_grade)
{
    long long can_move;
    struct node_page_refs *npf = &cpf->node_pfs[pair->hot_node];

    can_move = filter->flow_move_func(filter->ctrl, npf->size, pair->index);
    move_npf_to_list(npf, &memory_grade->cold_pages, can_move);
}

static void move_hot_pages(struct cslide_eng_params *eng_params, struct flow_ctrl *ctrl)
{
    struct page_filter filter;
    filter.flow_cal_func = flow_cal_hot_can_move;
    filter.flow_move_func = ctrl_hot_move;
    filter.flow_enough = is_hot_enough;
    filter.filter_policy = to_hot_policy;
    filter.ctrl = ctrl;
    filter.count_start = eng_params->loop * MAX_ACCESS_WEIGHT;
    filter.count_end = eng_params->hot_threshold - 1;
    filter.count_step = -1;
    do_filter(&filter, eng_params);
}

static void prefetch_hot_pages(struct cslide_eng_params *eng_params, struct flow_ctrl *ctrl)
{
    struct page_filter filter;
    filter.flow_cal_func = flow_cal_hot_can_prefetch;
    filter.flow_move_func = ctrl_prefetch_hot;
    filter.flow_enough = is_prefetch_enough;
    filter.filter_policy = to_hot_policy;
    filter.ctrl = ctrl;
    filter.count_start = eng_params->hot_threshold - 1;
    filter.count_end = -1;
    filter.count_step = -1;
    do_filter(&filter, eng_params);
}

static void move_cold_pages(struct cslide_eng_params *eng_params, struct flow_ctrl *ctrl)
{
    struct page_filter filter;
    filter.flow_cal_func = flow_cal_cold_can_move;
    filter.flow_move_func = ctrl_cold_move;
    filter.flow_enough = is_cold_enough;
    filter.filter_policy = to_cold_policy;
    filter.ctrl = ctrl;
    filter.count_start = 0;
    filter.count_end = eng_params->hot_threshold;
    filter.count_step = 1;
    do_filter(&filter, eng_params);
}

static int cslide_filter_pfs(struct cslide_eng_params *eng_params)
{
    struct flow_ctrl ctrl;

    if (init_flow_ctrl(&ctrl, eng_params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "init_flow_ctrl fail\n");
        return -1;
    }

    move_hot_pages(eng_params, &ctrl);
    prefetch_hot_pages(eng_params, &ctrl);
    move_cold_pages(eng_params, &ctrl);

    destroy_flow_ctrl(&ctrl);
    return 0;
}

static int cslide_get_vmas(struct cslide_pid_params *pid_params)
{
    struct cslide_task_params *task_params = pid_params->task_params;
    struct vma *vma = NULL;
    char pid[PID_STR_MAX_LEN] = {0};
    uint64_t i;
    int ret = -1;

    if (snprintf_s(pid, PID_STR_MAX_LEN, PID_STR_MAX_LEN - 1, "%u", pid_params->pid) <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "sprintf pid %u fail\n", pid_params->pid);
        return -1;
    }
    pid_params->vmas = get_vmas_with_flags(pid, task_params->vmflags_array, task_params->vmflags_num,
            task_params->anon_only);
    if (pid_params->vmas == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get vmas for %s fail\n", pid);
        return -1;
    }
    // avoid calloc for vma_pf with size 0 below
    // return success as vma may be created later
    if (pid_params->vmas->vma_cnt == 0) {
        etmemd_log(ETMEMD_LOG_WARN, "no vma detect for %s\n", pid);
        ret = 0;
        goto free_vmas;
    }

    pid_params->vma_pf = calloc(pid_params->vmas->vma_cnt, sizeof(struct vma_pf));
    if (pid_params->vma_pf == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc memory for vma_pf fail\n");
        goto free_vmas;
    }

    vma = pid_params->vmas->vma_list;
    for (i = 0; i < pid_params->vmas->vma_cnt; i++) {
        pid_params->vma_pf[i].vma = vma;
        vma = vma->next;
    }
    return 0;

free_vmas:
    free_vmas(pid_params->vmas);
    pid_params->vmas = NULL;
    return ret;
}

static void cslide_free_vmas(struct cslide_pid_params *params)
{
    uint64_t i;

    if (params->vmas == NULL) {
        return;
    }

    for (i = 0; i < params->vmas->vma_cnt; i++) {
        clean_page_refs_unexpected(&params->vma_pf[i].page_refs);
    }
    free(params->vma_pf);
    params->vma_pf = NULL;
    free_vmas(params->vmas);
    params->vmas = NULL;
}

static int cslide_scan_vmas(struct cslide_pid_params *params)
{
    char pid[PID_STR_MAX_LEN] = {0};
    struct vmas *vmas = params->vmas;
    struct vma *vma = NULL;
    struct vma_pf *vma_pf = NULL;
    FILE *scan_fp = NULL;
    struct walk_address walk_address;
    uint64_t i;
    int fd;
    struct cslide_task_params *task_params = params->task_params;
    struct ioctl_para ioctl_para = {
        .ioctl_cmd = IDLE_SCAN_ADD_FLAGS,
        .ioctl_parameter = task_params->scan_flags,
    };

    if (snprintf_s(pid, PID_STR_MAX_LEN, PID_STR_MAX_LEN - 1, "%u", params->pid) <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "snpintf pid %u fail\n", params->pid);
        return -1;
    }

    scan_fp = etmemd_get_proc_file(pid, IDLE_SCAN_FILE, "r");
    if (scan_fp == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "open %s file for pid %u fail\n", IDLE_SCAN_FILE, params->pid);
        return -1;
    }

    if (task_params->scan_flags != 0 && etmemd_send_ioctl_cmd(scan_fp, &ioctl_para) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "etmemd send_ioctl_cmd %s file for pid %u fail\n", IDLE_SCAN_FILE, params->pid);
        fclose(scan_fp);
        return -1;
    }

    fd = fileno(scan_fp);
    if (fd == -1) {
        fclose(scan_fp);
        etmemd_log(ETMEMD_LOG_ERR, "task %u fileno file fail for %s\n", params->pid, IDLE_SCAN_FILE);
        return -1;
    }
    for (i = 0; i < vmas->vma_cnt; i++) {
        vma_pf = &params->vma_pf[i];
        vma = vma_pf->vma;
        walk_address.walk_start = vma->start;
        walk_address.walk_end = vma->end;
        if (walk_vmas(fd, &walk_address, &vma_pf->page_refs, NULL) == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "task %u scan vma start %llu end %llu fail\n",
                    params->pid, vma->start, vma->end);
            fclose(scan_fp);
            return -1;
        }
    }

    fclose(scan_fp);
    return 0;
}

// allocted data will be cleaned in cslide_main->cslide_clean_params
// ->cslide_free_vmas
static int cslide_do_scan(struct cslide_eng_params *eng_params)
{
    struct cslide_pid_params *iter = NULL;
    int i;

    factory_foreach_working_pid_params(iter, &eng_params->factory) {
        if (cslide_get_vmas(iter) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "cslide get vmas fail\n");
            return -1;
        }
    }

    for (i = 0; i < eng_params->loop; i++) {
        factory_foreach_working_pid_params(iter, &eng_params->factory) {
            if (iter->vmas == NULL) {
                continue;
            }
            if (cslide_scan_vmas(iter) != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "cslide scan vmas fail\n");
                return -1;
            }
        }
        sleep(eng_params->sleep);
    }

    return 0;
}

// error return -1; success return moved pages number
static int do_migrate_pages(unsigned int pid, struct page_refs *page_refs, int node)
{
    int batch_size = BATCHSIZE;
    int ret;
    void **pages = NULL;
    int *nodes = NULL;
    int *status = NULL;
    int actual_num = 0;
    int moved = -1;

    if (page_refs == NULL) {
        return 0;
    }

    nodes = malloc(sizeof(int) * batch_size);
    if (nodes == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc nodes fail\n");
        return -1;
    }

    status = malloc(sizeof(int) * batch_size);
    if (status == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc status fail\n");
        goto free_nodes;
    }

    pages = malloc(sizeof(void *) * batch_size);
    if (pages == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc pages fail\n");
        goto free_status;
    }

    moved = 0;
    while (page_refs != NULL) {
        pages[actual_num] = (void *)page_refs->addr;
        nodes[actual_num] = node;
        actual_num++;
        page_refs = page_refs->next;
        if (actual_num == batch_size || page_refs == NULL) {
            ret = move_pages(pid, actual_num, pages, nodes, status, MPOL_MF_MOVE_ALL);
            if (ret != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "task %d move_pages fail with %d errno %d\n", pid, ret, errno);
                moved = -1;
                break;
            }
            moved += actual_num;
            actual_num = 0;
        }
    }

    free(pages);
    pages = NULL;
free_status:
    free(status);
    status = NULL;
free_nodes:
    free(nodes);
    nodes = NULL;
    return moved;
}

static int migrate_single_task(unsigned int pid, const struct memory_grade *memory_grade, int hot_node, int cold_node)
{
    int moved;

    moved = do_migrate_pages(pid, memory_grade->cold_pages, cold_node);
    if (moved == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "task %u migrate cold pages fail\n", pid);
        return -1;
    }
    if (moved != 0) {
        etmemd_log(ETMEMD_LOG_INFO, "task %u move pages %llu KB from node %d to node %d\n",
                pid, HUGE_2M_TO_KB((unsigned int)moved), hot_node, cold_node);
    }

    moved = do_migrate_pages(pid, memory_grade->hot_pages, hot_node);
    if (moved == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "task %u migrate hot pages fail\n", pid);
        return -1;
    }
    if (moved != 0) {
        etmemd_log(ETMEMD_LOG_INFO, "task %u move pages %llu KB from node %d to %d\n",
                pid, HUGE_2M_TO_KB((unsigned int)moved), cold_node, hot_node);
    }

    return 0;
}

static int cslide_do_migrate(struct cslide_eng_params *eng_params)
{
    struct cslide_pid_params *iter = NULL;
    struct node_pair *pair = NULL;
    int bind_node, i;
    int ret = 0;

    factory_foreach_working_pid_params(iter, &eng_params->factory) {
        for (i = 0; i < eng_params->node_map.cur_num; i++) {
            pair = &eng_params->node_map.pair[i];
            bind_node = pair->hot_node < pair->cold_node ? pair->hot_node : pair->cold_node;
            if (numa_run_on_node(bind_node) != 0) {
                etmemd_log(ETMEMD_LOG_INFO, "fail to run on node %d to migrate memory\n", bind_node);
            }
            ret = migrate_single_task(iter->pid, &iter->memory_grade[i], pair->hot_node, pair->cold_node);
            if (ret != 0) {
                goto exit;
            }
        }
    }

exit:
    if (numa_run_on_node(-1) != 0) {
        etmemd_log(ETMEMD_LOG_INFO, "fail to run on all node after migrate memory\n");
    }
    return ret;
}

static void init_host_pages_info(struct cslide_eng_params *eng_params)
{
    int n;
    int node_num = eng_params->mem.node_num;
    struct node_pages_info *host_pages = eng_params->host_pages_info;

    for (n = 0; n < node_num; n++) {
        host_pages[n].cold = 0;
        host_pages[n].hot = 0;
    }
}

static void update_pages_info(struct cslide_eng_params *eng_params, struct cslide_pid_params *pid_params)
{
    int n, c;
    int t = eng_params->hot_threshold;
    int count = pid_params->count;
    int actual_t = t > count ? count + 1 : t;
    int node_num = pid_params->count_page_refs->node_num;
    struct node_pages_info *task_pages = pid_params->node_pages_info;
    struct node_pages_info *host_pages = eng_params->host_pages_info;

    for (n = 0; n < node_num; n++) {
        task_pages[n].cold = 0;
        task_pages[n].hot = 0;

        for (c = 0; c < actual_t; c++) {
            task_pages[n].cold += HUGE_2M_TO_KB(pid_params->count_page_refs[c].node_pfs[n].num);
        }
        for (; c <= count; c++) {
            task_pages[n].hot += HUGE_2M_TO_KB(pid_params->count_page_refs[c].node_pfs[n].num);
        }

        host_pages[n].cold += task_pages[n].cold;
        host_pages[n].hot += task_pages[n].hot;
    }
}

static void cslide_stat(struct cslide_eng_params *eng_params)
{
    struct cslide_pid_params *iter = NULL;

    pthread_mutex_lock(&eng_params->stat_mtx);
    init_host_pages_info(eng_params);
    factory_foreach_working_pid_params(iter, &eng_params->factory) {
        update_pages_info(eng_params, iter);
    }
    eng_params->stat_time = time(NULL);
    pthread_mutex_unlock(&eng_params->stat_mtx);
}

static int cslide_policy(struct cslide_eng_params *eng_params)
{
    struct cslide_pid_params *pid_params = NULL;
    int ret;

    factory_foreach_working_pid_params(pid_params, &eng_params->factory) {
        ret = cslide_count_node_pfs(pid_params);
        if (ret != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "count node page refs fail\n");
            return ret;
        }
    }

    // update pages info now, so cslide_filter_pfs can use this info
    cslide_stat(eng_params);

    return cslide_filter_pfs(eng_params);
}

static void cslide_clean_params(struct cslide_eng_params *eng_params)
{
    struct cslide_pid_params *iter = NULL;

    factory_foreach_pid_params(iter, &eng_params->factory) {
        // clean memory allocted in cslide_policy
        clean_pid_param(iter);
        // clean memory allocted in cslide_do_scan
        cslide_free_vmas(iter);
    }
}

static void destroy_cslide_eng_params(struct cslide_eng_params *params)
{
    free(params->host_pages_info);
    params->host_pages_info = NULL;
    destroy_factory(&params->factory);
    pthread_mutex_destroy(&params->stat_mtx);
    destroy_node_map(&params->node_map);
    destroy_sys_mem(&params->mem);
}

static int init_cslide_eng_params(struct cslide_eng_params *params)
{
    int node_num;

    if (init_sys_mem(&params->mem) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "init system memory fail\n");
        return -1;
    }

    if (init_node_map(&params->node_map, params->mem.node_num) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "init_node_map fail\n");
        goto destroy_sys_mem;
    }

    if (pthread_mutex_init(&params->stat_mtx, NULL) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "init stat mutex fail\n");
        goto destroy_node_map;
    }

    if (init_factory(&params->factory) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "init params factory fail\n");
        goto destroy_stat_mtx;
    }

    node_num = params->mem.node_num;
    params->host_pages_info = calloc(node_num, sizeof(struct node_pages_info));
    if (params->host_pages_info == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc host_pages_info fail\n");
        goto destroy_factory;
    }

    return 0;

destroy_factory:
    destroy_factory(&params->factory);

destroy_stat_mtx:
    pthread_mutex_destroy(&params->stat_mtx);

destroy_node_map:
    destroy_node_map(&params->node_map);

destroy_sys_mem:
    destroy_sys_mem(&params->mem);
    return -1;
}

static void *cslide_main(void *arg)
{
    struct cslide_eng_params *eng_params = (struct cslide_eng_params *)arg;
    struct sys_mem *mem = NULL;

    // only invalid pthread id or deatch more than once will cause error
    // so no need to check return value of pthread_detach
    (void)pthread_detach(pthread_self());

    while (true) {
        factory_update_pid_params(&eng_params->factory);
        if (eng_params->finish) {
            etmemd_log(ETMEMD_LOG_DEBUG, "cslide task is stopping...\n");
            break;
        }
        if (factory_working_empty(&eng_params->factory)) {
            goto next;
        }

        mem = &eng_params->mem;
        if (get_sys_mem(mem) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "get system meminfo fail\n");
            goto next;
        }

        if (cslide_do_scan(eng_params) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "cslide_do_scan fail\n");
            goto next;
        }

        if (cslide_policy(eng_params) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "cslide_policy fail\n");
            goto next;
        }

        if (cslide_do_migrate(eng_params) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "cslide_do_migrate fail\n");
            goto next;
        }

next:
        sleep(eng_params->interval);
        cslide_clean_params(eng_params);
    }

    factory_update_pid_params(&eng_params->factory);
    destroy_cslide_eng_params(eng_params);
    free(eng_params);
    return NULL;
}

static int cslide_alloc_pid_params(struct engine *eng, struct task_pid **tk_pid)
{
    struct cslide_eng_params *eng_params = (struct cslide_eng_params *)eng->params;
    unsigned pid = (*tk_pid)->pid;
    struct cslide_pid_params *pid_params = NULL;

    pid_params = alloc_pid_params(eng_params);
    if (pid_params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc cslide pid params fail\n");
        return -1;
    }

    pid_params->pid = pid;
    pid_params->eng_params = eng_params;
    pid_params->task_params = (*tk_pid)->tk->params;
    (*tk_pid)->params = pid_params;
    return 0;
}

static void cslide_free_pid_params(struct engine *eng, struct task_pid **tk_pid)
{
    struct cslide_eng_params *eng_params = (struct cslide_eng_params *)eng->params;

    if ((*tk_pid)->params != NULL) {
        /* clear pid params in factory_update_pid_params in cslide_main */
        factory_free_pid_params(&eng_params->factory, (*tk_pid)->params);
        (*tk_pid)->params = NULL;
    }
}

static int cslide_start_task(struct engine *eng, struct task *tk)
{
    struct cslide_eng_params *eng_params = (struct cslide_eng_params *)eng->params;
    struct task_pid *task_pid = NULL;

    for (task_pid = tk->pids; task_pid != NULL; task_pid = task_pid->next) {
        factory_add_pid_params(&eng_params->factory, task_pid->params);
    }

    return 0;
}

static void cslide_stop_task(struct engine *eng, struct task *tk)
{
    struct cslide_eng_params *eng_params = (struct cslide_eng_params *)eng->params;
    struct task_pid *task_pid = NULL;

    for (task_pid = tk->pids; task_pid != NULL; task_pid = task_pid->next) {
        factory_remove_pid_params(&eng_params->factory, task_pid->params);
    }
}

static char *get_time_stamp(time_t *t)
{
    struct tm *lt = NULL;
    char *ts = NULL;
    size_t len;

    lt = localtime(t);
    if (lt == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get local time fail\n");
        return NULL;
    }

    ts = asctime(lt);
    if (ts == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get asctime fail\n");
        return NULL;
    }

    len = strlen(ts);
    if (ts[len - 1] == '\n') {
        ts[len - 1] = '\0';
    }
    return ts;
}

static int show_task_pages(void *params, int fd)
{
    struct cslide_pid_params *pid_params = (struct cslide_pid_params *)params;
    struct cslide_eng_params *eng_params = pid_params->eng_params;
    int node_num = pid_params->count_page_refs->node_num;
    struct node_pages_info *info = pid_params->node_pages_info;
    char *time_str = NULL;
    int n;

    time_str = get_time_stamp(&eng_params->stat_time);
    if (time_str != NULL) {
        dprintf_all(fd, "[%s] ", time_str);
    }
    dprintf_all(fd, "task %d pages info (KB):\n", pid_params->pid);
    dprintf_all(fd, "%5s %10s %10s %10s\n", "node", "used", "hot", "cold");
    for (n = 0; n < node_num; n++) {
        dprintf_all(fd, "%5d %10d %10d %10d\n", n,
                    info[n].hot + info[n].cold, info[n].hot, info[n].cold);
    }
    return 0;
}

static struct cslide_cmd_item g_task_cmd_items[] = {
    {"showtaskpages", show_task_pages},
};

static int show_host_pages(void *params, int fd)
{
    struct cslide_eng_params *eng_params = (struct cslide_eng_params *)params;
    char *time_str = NULL;
    int node_num = eng_params->mem.node_num;
    int n;
    uint32_t total;
    struct node_pages_info *info = eng_params->host_pages_info;

    time_str = get_time_stamp(&eng_params->stat_time);
    if (time_str != NULL) {
        dprintf_all(fd, "[%s] ", time_str);
    }
    dprintf_all(fd, "host pages info (KB):\n");
    dprintf_all(fd, "%5s %10s %10s %10s %10s\n", "node", "total", "used", "hot", "cold");
    for (n = 0; n < node_num; n++) {
        total = eng_params->mem.node_mem[n].huge_total / 1024;
        dprintf_all(fd, "%5d %10d %10d %10d %10d\n",
                    n, total, info[n].hot + info[n].cold, info[n].hot, info[n].cold);
    }

    return 0;
}

struct cslide_cmd_item g_host_cmd_items[] = {
    {"showhostpages", show_host_pages},
};

static struct cslide_cmd_item *find_cmd_item(struct cslide_cmd_item *items, unsigned n, char *cmd)
{
    unsigned i;

    for (i = 0; i < n; i++) {
        if (strcmp(cmd, items[i].name) == 0) {
            return &items[i];
        }
    }

    return NULL;
}

static int cslide_engine_do_cmd(struct engine *eng, struct task *tk, char *cmd, int fd)
{
    struct cslide_eng_params *eng_params = (struct cslide_eng_params *)eng->params;
    struct cslide_pid_params *pid_params = NULL;
    struct cslide_cmd_item *item = NULL;
    int ret;

    if (factory_working_empty(&eng_params->factory)) {
        etmemd_log(ETMEMD_LOG_ERR, "no working pid under this cslide engine\n");
        return -1;
    }

    item = find_cmd_item(g_host_cmd_items, ARRAY_SIZE(g_host_cmd_items), cmd);
    if (item != NULL) {
        pthread_mutex_lock(&eng_params->stat_mtx);
        ret = item->func(eng_params, fd);
        pthread_mutex_unlock(&eng_params->stat_mtx);
        return ret;
    }

    item = find_cmd_item(g_task_cmd_items, ARRAY_SIZE(g_task_cmd_items), cmd);
    if (item == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "cslide cmd %s is not supported\n", cmd);
        return -1;
    }
    if (tk == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "task for cslide cmd %s not found\n", cmd);
        return -1;
    }

    if (tk->pids == NULL || tk->pids->params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "task %s for cslide cmd %s not started\n", tk->name, cmd);
        return -1;
    }

    pid_params = (struct cslide_pid_params *)tk->pids->params;
    pthread_mutex_lock(&eng_params->stat_mtx);
    ret = item->func(pid_params, fd);
    pthread_mutex_unlock(&eng_params->stat_mtx);
    return ret;
}

static int fill_task_anon_only(void *obj, void *val)
{
    int ret = 0;
    struct cslide_task_params *params = (struct cslide_task_params *)obj;
    char *anon_only = (char *)val;

    if (strcmp(anon_only, "yes") == 0) {
        params->anon_only = true;
    } else if (strcmp(anon_only, "no") == 0) {
        params->anon_only = false;
    } else {
        etmemd_log(ETMEMD_LOG_ERR, "only_anon : not support %s\n", anon_only);
        etmemd_log(ETMEMD_LOG_ERR, "only_anon : only support yes/no\n");
        ret = -1;
    }
    free(val);
    return ret;
}

static int fill_task_vm_flags(void *obj, void *val)
{
    struct cslide_task_params *params = (struct cslide_task_params *)obj;
    char *vm_flags = (char *)val;

    params->vmflags_num = split_vmflags(&params->vmflags_array, vm_flags);
    if (params->vmflags_num <= 0) {
        free(val);
        etmemd_log(ETMEMD_LOG_ERR, "fill vm flags fail\n");
        return -1;
    }

    params->vmflags_str = vm_flags;
    if (params->vmflags_num != 1 || strcmp(params->vmflags_array[0], "ht") != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "cslide only work with ht set\n");
        return -1;
    }
    return 0;
}

static int fill_task_scan_flags(void *obj, void *val)
{
    struct cslide_task_params *params = (struct cslide_task_params *)obj;
    char *ign_host = (char *)val;
    int ret = 0;

    params->scan_flags |= SCAN_AS_HUGE;

    if (strcmp(ign_host, "yes") == 0) {
        params->scan_flags |= SCAN_IGN_HOST;
    } else if (strcmp(ign_host, "no") != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "ign_host : not support %s\n", ign_host);
        etmemd_log(ETMEMD_LOG_ERR, "ign_host : only support yes/no\n");
        ret = -1;
    }

    free(val);
    return ret;
}

static struct config_item g_cslide_task_config_items[] = {
    {"vm_flags", STR_VAL, fill_task_vm_flags, false},
    {"anon_only", STR_VAL, fill_task_anon_only, false},
    {"ign_host", STR_VAL, fill_task_scan_flags, false},
};

static int cslide_fill_task(GKeyFile *config, struct task *tk)
{
    struct cslide_task_params *params = calloc(1, sizeof(struct cslide_task_params));

    if (params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc cslide task params fail\n");
        return -1;
    }

    if (parse_file_config(config, TASK_GROUP, g_cslide_task_config_items,
                          ARRAY_SIZE(g_cslide_task_config_items), (void *)params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "cslide fill task params fail\n");
        goto exit;
    }

    tk->params = params;
    if (etmemd_get_task_pids(tk, false) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "cslide fail to get task pids\n");
        tk->params = NULL;
        goto exit;
    }
    return 0;

exit:
    clear_task_params(params);
    free(params);
    return -1;
}

static void cslide_clear_task(struct task *tk)
{
    /* clear cslide task params when clear connected cslide pid params */
    etmemd_free_task_pids(tk);
    tk->params = NULL;
}

static int fill_node_pair(void *obj, void *val)
{
    struct cslide_eng_params *params = (struct cslide_eng_params *)obj;
    char *node_pair_str = (char *)val;
    char *pair = NULL;
    char *saveptr_pair = NULL;
    char *hot_node_str = NULL;
    char *cold_node_str = NULL;
    char *saveptr_node = NULL;
    int hot_node, cold_node;
    struct node_map *map = &params->node_map;
    struct node_verifier nv;
    char *pair_delim = " ;";
    char *node_delim = " ,";
    int ret = -1;

    if (init_node_verifier(&nv, params->mem.node_num) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "init_node_verifier fail\n");
        free(val);
        return ret;
    }

    for (pair = strtok_r(node_pair_str, pair_delim, &saveptr_pair); pair != NULL;
            pair = strtok_r(NULL, pair_delim, &saveptr_pair)) {
        hot_node_str = strtok_r(pair, node_delim, &saveptr_node);
        if (hot_node_str == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "parse hot node failed\n");
            goto err;
        }

        cold_node_str = strtok_r(NULL, node_delim, &saveptr_node);
        if (cold_node_str == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "parse cold node failed\n");
            goto err;
        }

        if (get_int_value(hot_node_str, &hot_node) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "transfer hot node %s to integer fail\n", hot_node_str);
            goto err;
        }

        if (get_int_value(cold_node_str, &cold_node) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "transfer cold node %s to integer fail\n", cold_node_str);
            goto err;
        }

        if (!is_node_valid(&nv, hot_node) || !is_node_valid(&nv, cold_node)) {
            etmemd_log(ETMEMD_LOG_ERR, "node %d(hot)->%d(cold) invalid\n", hot_node, cold_node);
            goto err;
        }

        if (add_node_pair(map, cold_node, hot_node) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "add %d(hot)->%d(cold) fail\n", hot_node, cold_node);
            goto err;
        }
    }

    if (has_node_unmap(&nv)) {
        etmemd_log(ETMEMD_LOG_ERR, "there is node unmap\n");
        goto err;
    }
    ret = 0;

err:
    free(val);
    destroy_node_verifier(&nv);
    return ret;
}

static int fill_hot_threshold(void *obj, void *val)
{
    struct cslide_eng_params *params = (struct cslide_eng_params *)obj;
    int t = parse_to_int(val);

    if (t < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "config hot threshold %d not valid\n", t);
        return -1;
    }

    if (t > params->loop * MAX_ACCESS_WEIGHT + 1) {
        // limit hot_threshold to avoid overflow in do_filter
        t = params->loop * MAX_ACCESS_WEIGHT + 1;
    }

    params->hot_threshold = t;
    return 0;
}

static int fill_hot_reserve(void *obj, void *val)
{
    struct cslide_eng_params *params = (struct cslide_eng_params *)obj;
    int hot_reserve = parse_to_int(val);

    if (hot_reserve < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "config hot reserve %d not valid\n", hot_reserve);
        return -1;
    }

    params->hot_reserve = hot_reserve;
    return 0;
}

static int fill_mig_quota(void *obj, void *val)
{
    struct cslide_eng_params *params = (struct cslide_eng_params *)obj;
    int mig_quota = parse_to_int(val);

    if (mig_quota < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "config mig quota %d not valid\n", mig_quota);
        return -1;
    }

    params->mig_quota = mig_quota;
    return 0;
}

static struct config_item cslide_eng_config_items[] = {
    {"node_pair", STR_VAL, fill_node_pair, false},
    {"hot_threshold", INT_VAL, fill_hot_threshold, false},
    {"node_mig_quota", INT_VAL, fill_mig_quota, false},
    {"node_hot_reserve", INT_VAL, fill_hot_reserve, false},
};

static int cslide_fill_eng(GKeyFile *config, struct engine *eng)
{
    struct cslide_eng_params *params = calloc(1, sizeof(struct cslide_eng_params));
    struct page_scan *page_scan = (struct page_scan *)eng->proj->scan_param;
    
    if (params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc cslide engine params fail\n");
        return -1;
    }

    if (init_cslide_eng_params(params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "init cslide engine params fail\n");
        goto free_eng_params;
    }

    params->loop = page_scan->loop;
    params->interval = page_scan->interval;
    params->sleep = page_scan->sleep;
    if (parse_file_config(config, ENG_GROUP, cslide_eng_config_items,
        ARRAY_SIZE(cslide_eng_config_items), (void *)params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "cslide fill engine params fail\n");
        goto destroy_eng_params;
    }

    eng->params = params;
    if (pthread_create(&params->worker, NULL, cslide_main, params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "start cslide main worker fail\n");
        goto destroy_eng_params;
    }

    return 0;

destroy_eng_params:
    destroy_cslide_eng_params(params);
free_eng_params:
    free(params);
    return -1;
}

static void cslide_clear_eng(struct engine *eng)
{
    struct cslide_eng_params *eng_params = eng->params;
    /* clear cslide engine params in cslide_main */
    eng_params->finish = true;
    eng->params = NULL;
}

struct engine_ops g_cslide_eng_ops = {
    .fill_eng_params = cslide_fill_eng,
    .clear_eng_params = cslide_clear_eng,
    .fill_task_params = cslide_fill_task,
    .clear_task_params = cslide_clear_task,
    .alloc_pid_params = cslide_alloc_pid_params,
    .free_pid_params = cslide_free_pid_params,
    .start_task = cslide_start_task,
    .stop_task = cslide_stop_task,
    .eng_mgt_func = cslide_engine_do_cmd,
};

int fill_engine_type_cslide(struct engine *eng, GKeyFile *config)
{
    eng->ops = &g_cslide_eng_ops;
    eng->engine_type = CSLIDE_ENGINE;
    eng->name = "cslide";
    return 0;
}
