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
 * Create: 2021-11-29
 * Description: construct a progress test for etmemd cslide engine
 ******************************************************************************/

#include <numaif.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FLAGS               (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)
#define PROTECTION          (PROT_READ | PROT_WRITE)
#define PATH_MAX            1024
#define BYTE_TO_MB_SHIFT    20
#define SLEEP_TIME          1000

void *mmap_hugepage(size_t len)
{
    return mmap(NULL, len, PROTECTION, FLAGS, -1, 0);
}

void fill_random(void *addr, size_t len)
{
    size_t i;
    for (i = 0; i < len / sizeof(int); i++) {
        ((int *)addr)[i] = rand();
    }
}

int main(int argc, char **argv)
{
    void *src_addr = NULL;
    void *dst_addr = NULL;
    unsigned long nodemask;
    uint64_t start, end, total = 0;
    int time = 1;
    int i;
    char c;
    long ret;
    int dst_node;
    long long len;
    size_t j;
    int is_busy;

    if (argc > 3) {
        dst_node = atoi(argv[1]);
        len = (long long)atoi(argv[2]) << BYTE_TO_MB_SHIFT;
        is_busy = atoi(argv[3]);
    } else {
        printf("%s node size(MB) is busy\n", argv[0]);
        return -1;
    }

    nodemask = (unsigned long)1 << dst_node;
    if ((ret = set_mempolicy(MPOL_BIND, &nodemask, 8)) != 0) {
        printf("set_mempolicy for aep fail with %ld\n", ret);
        return -1;
    }

    dst_addr = mmap_hugepage(len);
    if (dst_addr == NULL) {
        printf("mmap fail\n");
        return -1;
    }
    fill_random(dst_addr, len);
    printf("finish page fault\n");

    if (!is_busy) {
        sleep(SLEEP_TIME);
    }

    while (1) {
        fill_random(dst_addr, len);
    }

    return 0;
}
