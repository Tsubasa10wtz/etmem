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
 * Author: liubo
 * Create: 2021-11-29
 * Description: construct a progress test for etmemd slide engine
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <memory.h>

#include "securec.h"

#define VMA_FLAG    203
#define SLEEP_TIME  2

int main(int argc, char *argv[])
{
    char *memory_with_flag = NULL;
    char *memory_no_flag = NULL;
    char i = 0;
    int ret = -1;
    /* get memory size: 1GB for test */
    size_t memory_len = (1UL * 1024) * 1024 * 1024;

    memory_with_flag = (char *)mmap(NULL, memory_len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (memory_with_flag == MAP_FAILED) {
        perror("map mem");
        memory_with_flag = NULL;
        return -1;
    }

    ret = madvise(memory_with_flag, memory_len, VMA_FLAG);
    if (ret != 0) {
        printf("madvise error.\n");
        goto free_memory_with_flag;
    }

    ret = memset_s(memory_with_flag, memory_len, '0', memory_len);
    if (ret != EOK) {
        printf("memset for memory_with flag fail, ret: %d err(%s)\n", ret, strerror(errno));
        goto free_memory_with_flag;
    }

    memory_no_flag = (char *)mmap(NULL, memory_len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (memory_no_flag == MAP_FAILED) {
        perror("map mem");
        goto free_memory_with_flag;
    }

    ret = memset_s(memory_no_flag, memory_len, '0', memory_len);
    if (ret != EOK) {
        printf("memset for memory_no_flag flag fail, ret: %d err(%s)\n", ret, strerror(errno));
        goto free_all_memory;
    }

    while (1) {
        ret = memset_s(memory_no_flag, memory_len, i, memory_len);
        if (ret != EOK) {
            printf("memset for memory_no_flag flag fail\n");
            goto free_all_memory;
        }

        sleep(SLEEP_TIME);
        i++;
    }

free_all_memory:
    ret = munmap(memory_no_flag, memory_len);
    if (ret != 0) {
        printf("release memory for memory_no_flag failed.\n");
        return -1;
    }

free_memory_with_flag:
    ret = munmap(memory_with_flag, memory_len);
    if (ret != 0) {
        printf("release memory for memory_with_flag failed.\n");
        return -1;
    }

    return 0;
}
