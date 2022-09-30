/* *****************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: yangxin
 * Create: 2022-09-26
 * Description: Allocate memory for memRouter Unit test
 * **************************************************************************** */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/un.h>
#include <errno.h>
#include <numa.h>
#include <numaif.h>
#include <time.h>
#include "alloc_memory.h"

#define MAX_LISTENQ_NUM 1
#define RESP_MSG_MAX_LEN 10
#define PATH_MAX_LEN 127
#define USWAP_ERROR (-1)

#define PID_MAX_FILE "/proc/sys/kernel/pid_max"

struct memdcd_message *alloc_memory(int total_len, int *msg_num)
{
    int last_msg_len = total_len % 512;
    int pagesize = getpagesize();
    int *pages = NULL;
    int *page_start = NULL;
    int *page_end = NULL;
    struct memdcd_message *ret = NULL;
    *msg_num = total_len / 512;
    if (last_msg_len == 0) {
        last_msg_len = 512;
    } else {
        *msg_num += 1;
    }
    
    ret = (struct memdcd_message *)malloc(sizeof(struct memdcd_message) * *msg_num);
    if (ret == NULL) {
        printf("malloc error in alloc_memory");
        return NULL;
    }
    ret[0].cmd_type = MEMDCD_CMD_MEM;
    ret[*msg_num - 1].cmd_type = MEMDCD_CMD_MEM;
    if (posix_memalign((void **)&page_start, pagesize, pagesize * MAX_VMA_NUM) != 0) {
        printf("memalign allocate memmory failed");
        free(ret);
        return NULL;
    }
    memset(page_start, 1, pagesize * MAX_VMA_NUM);
    ret[0].memory_msg.pid = getpid();
    ret[0].memory_msg.enable_uswap = 1;
    ret[0].memory_msg.vma.length = sizeof(struct vma_addr_with_count) * MAX_VMA_NUM;
    ret[0].memory_msg.vma.total_length = total_len;

    ret[0].memory_msg.vma.status = MEMDCD_SEND_START;
    for (int i = 0; i < MAX_VMA_NUM; i++) {
        ret[0].memory_msg.vma.vma_addrs[i].vma.start_addr = (long)(page_start + i * pagesize);
        ret[0].memory_msg.vma.vma_addrs[i].count = 1;
        ret[0].memory_msg.vma.vma_addrs[i].vma.vma_len = 0x1000;
    }
    if (total_len <= 512) {
        ret[0].memory_msg.vma.length = sizeof(struct vma_addr_with_count) * total_len;
        return ret;
    }
    for (int i = 1; i < *msg_num - 1; i++) {
        if (posix_memalign((void **)&pages, pagesize, pagesize * MAX_VMA_NUM) != 0) {
            printf("memalign allocate memmory failed");
            for (int j = 0; j < i; j++) {
                free(ret[j].memory_msg.vma.vma_addrs[0].vma.start_addr);
            }
            free(ret);
            return NULL;
        }
        memset(pages, 1, pagesize * MAX_VMA_NUM);
        ret[i].memory_msg.pid = getpid();
        ret[i].memory_msg.enable_uswap = 1;
        ret[i].memory_msg.vma.length = sizeof(struct vma_addr_with_count) * MAX_VMA_NUM;
        ret[i].memory_msg.vma.total_length = total_len;
        ret[i].memory_msg.vma.status = MEMDCD_SEND_PROCESS;
        for (int j = 0; j < MAX_VMA_NUM; j++) {
            ret[i].memory_msg.vma.vma_addrs[j].vma.start_addr = (long)(pages + j * pagesize);
            ret[i].memory_msg.vma.vma_addrs[j].count = 1;
            ret[i].memory_msg.vma.vma_addrs[j].vma.vma_len = 0x1000;
        }
    }
    if (posix_memalign((void **)&page_end, pagesize, pagesize * last_msg_len) != 0) {
        printf("memalign allocate memmory failed");
        for (int i = 0; i < *msg_num - 1; i++) {
            free(ret[i].memory_msg.vma.vma_addrs[0].vma.start_addr);
        }
        free(ret);
        return NULL;
    }
    memset(page_end, 1, pagesize * last_msg_len);
    ret[*msg_num - 1].memory_msg.pid = getpid();
    ret[*msg_num - 1].memory_msg.enable_uswap = 1;
    ret[*msg_num - 1].memory_msg.vma.length = sizeof(struct vma_addr_with_count) * last_msg_len;
    ret[*msg_num - 1].memory_msg.vma.total_length = total_len;
    ret[*msg_num - 1].memory_msg.vma.status = MEMDCD_SEND_END;
    for (int i = 0; i < last_msg_len; i++) {
        ret[*msg_num - 1].memory_msg.vma.vma_addrs[i].vma.start_addr = (long)(page_end + i * pagesize);
        ret[*msg_num - 1].memory_msg.vma.vma_addrs[i].count = 1;
        ret[*msg_num - 1].memory_msg.vma.vma_addrs[i].vma.vma_len = 0x1000;
    }

    return ret;
}

void free_memory(struct memdcd_message *msg, int msg_num)
{
    for (int i = 0; i < msg_num; i++) {
        free(msg[i].memory_msg.vma.vma_addrs[0].vma.start_addr);
    }
    free(msg);
}

int format_msg(struct memdcd_message *msg, int msg_num, struct vma_addr **vma_msg)
{
    if (msg == NULL || msg_num <= 0) {
        return -1;
    }

    int vma_total = msg[0].memory_msg.vma.total_length;
    *vma_msg = (struct vma_addr *)malloc(sizeof(struct vma_addr) * vma_total);
    if (vma_msg == NULL) {
        printf("malloc error in format_msg");
        return NULL;
    }

    long idx = 0;
    for (int i = 0; i < msg_num; i++) {
        struct swap_vma_with_count msg_vmas = msg[i].memory_msg.vma;
        int vma_num = msg_vmas.length / sizeof(struct vma_addr_with_count);

        for (int j = 0; j < vma_num; j++) {
            (*vma_msg)[j + idx].start_addr = msg_vmas.vma_addrs[j].vma.start_addr;
            (*vma_msg)[j + idx].vma_len = msg_vmas.vma_addrs[j].vma.vma_len;
        }
        idx += msg_vmas.length;
    }
    return vma_total;
}

void dump_msg(struct vma_addr *msg, long len)
{
    for (int i = 0; i < len; i++) {
        long _addr = msg[i].start_addr;
        long _len = msg[i].vma_len;
        printf("!!!--->[%d] vma: %lx len:%u<----!!!\n", i, _addr, _len);
    }
    return;
}

int cmp_msg(struct vma_addr *msg1, long len1, struct vma_addr *msg2, long len2)
{
    if (len1 != len2) {
        printf("!!!---> len1:%d, len2:%d <----!!!", len1, len2);
        goto NOT_EQUAL;
    }
    printf("!!!---> len1:%u, len2:%u <----!!!\n", len1, len2);
    for (int i = 0; i < len1; i++) {
        long _addr1 = msg1[i].start_addr;
        long _addr2 = msg2[i].start_addr;
        long _len1 = msg1[i].vma_len;
        long _len2 = msg2[i].vma_len;
        if ((_addr1 != _addr2) || (_len1 != _len2)) {
            printf("!!!---> vma1: %lx len1:%u, vma2: %lx len2:%u <----!!!\n", _addr1, _len1, _addr2, _len2);
            goto NOT_EQUAL;
        }
    }
    printf("******msg1 equal to msg2********\n");
    return 0;

NOT_EQUAL:
    printf("******msg1 not equal to msg2********\n");
    return 1;
}

static int SOCKET_RECV_BUF_LEN = 0;

int init_socket(void)
{
    int socket_fd;
    int addrlen;
    struct sockaddr_un addr;
    char abstract_path[PATH_MAX_LEN] = {0};
    pid_t pid = getpid();
    SOCKET_RECV_BUF_LEN = sysconf(_SC_PAGESIZE);

    socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        printf("create socket failed\n");
        return -1;
    }

    bzero(&addr, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;
    snprintf(abstract_path, sizeof(abstract_path), "userswap%d.sock", pid);
    memcpy(addr.sun_path + 1, abstract_path, strlen(abstract_path) + 1);
    addrlen = sizeof(addr.sun_family) + strlen(abstract_path) + 1;
    if (bind(socket_fd, (struct sockaddr *)&addr, addrlen) < 0) {
        printf("bind socket failed\n");
        close(socket_fd);
        return -1;
    }

    listen(socket_fd, MAX_LISTENQ_NUM);
    return socket_fd;
}

int sock_handle_rec(int fd, struct swap_vma *swap_vma)
{
    int client_fd;
    int readbytes;
    struct sockaddr_un clientun;
    socklen_t clientun_len = sizeof(clientun);

    client_fd = accept(fd, (struct sockaddr *)&clientun, &clientun_len);
    if (client_fd < 0) {
        return -1;
    }

    readbytes = read(client_fd, swap_vma, SOCKET_RECV_BUF_LEN);
    if (readbytes <= 0) {
        close(client_fd);
        return -1;
    }

    return client_fd;
}

int sock_handle_respond(int client_fd, int result)
{
    int writebytes;
    char buff[RESP_MSG_MAX_LEN] = {0};

    if (client_fd < 0) {
        return -1;
    }

    if (result == 0) {
        snprintf(buff, sizeof(buff), "success");
    } else {
        snprintf(buff, sizeof(buff), "failed");
    }

    writebytes = write(client_fd, buff, (strlen(buff) + 1));
    if (writebytes != (strlen(buff) + 1)) {
        close(client_fd);
        return -1;
    }

    close(client_fd);
    return 0;
}

int simulate_uswap(int pid, struct vma_addr **recv_msg)
{
    struct swap_vma swap_vma;
    size_t len;
    int swapout_nums;
    int succ_count = 0;
    int socket_fd = -1;
    int client_fd = -1;
    int ret = 0;
    int connect_flag = 0;
    clock_t start, end;

    socket_fd = init_socket();
    if (socket_fd < 0) {
        printf("init socket fd error");
        return socket_fd;
    }
    prctl(PR_SET_NAME, "uswap-swapout", 0, 0, 0);
    start = clock();
    while (1) {
        client_fd = sock_handle_rec(socket_fd, &swap_vma);
        end = clock();
        // wait 1s
        if (((end - start) / CLOCKS_PER_SEC) > 1) {
            printf("recv time's up!\n");
            break;
        }
        if (client_fd <= 0) {
            continue;
        }
        if (connect_flag == 0) {
            connect_flag = 1;
            start = clock();
        }
        swapout_nums = swap_vma.length / sizeof(struct vma_addr);
        ret = swapout_nums;
        *recv_msg = (struct vma_addr *)malloc(sizeof(struct vma_addr) * ret);
        if (recv_msg == NULL) {
            printf("malloc error in simulate_uswap");
            continue;
        }
        memcpy(*recv_msg, swap_vma.vma_addrs, swap_vma.length);
        sock_handle_respond(client_fd, 0);
        break;
    }

    close(socket_fd);
    return ret;
}

int get_pid_max(void)
{
    FILE *fp;
    char buf[12] = {0};

    fp = fopen(PID_MAX_FILE, "r");
    if (fp == NULL) {
        return -1;
    }
    if (fread(buf, sizeof(buf), 1, fp) == 0) {
        if (feof(fp) == 0) {
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return atoi(buf);
}
