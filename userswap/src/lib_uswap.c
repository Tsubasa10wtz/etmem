/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * userswap licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: liuyongqiang
 * Create: 2020-11-06
 * Description: userswap interface definition.
 ******************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <execinfo.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/userfaultfd.h>
#include "uswap_server.h"
#include "uswap_log.h"
#include "uswap_api.h"

#define MAP_REPLACE 0x1000000
#ifndef UFFDIO_REGISTER_MODE_USWAP
#define UFFDIO_REGISTER_MODE_USWAP (1 << 2)
#endif
#define MMAP_RETVAL_DIRTY_MASK 0x01L
#define MAX_TRY_NUMS 10

struct uswap_dev {
    char name[MAX_USWAP_NAME_LEN];
    struct uswap_operations *ops;
    bool enabled;
    bool alive;
    int uffd;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

static struct uswap_dev g_dev = {
    .name = "",
    .ops = NULL,
    .enabled = false,
    .alive = false,
    .uffd = -1,
    .cond = PTHREAD_COND_INITIALIZER,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

static size_t get_page_size(void)
{
    static size_t page_size = 0;
    if (page_size != 0) {
        return page_size;
    }

    page_size = sysconf(_SC_PAGESIZE);
    return page_size;
}

static bool is_uswap_enabled(void)
{
    return g_dev.enabled;
}

static bool is_uswap_threads_alive(void)
{
    return g_dev.alive;
}

static int set_uswap_uffd(int uffd)
{
    if (g_dev.uffd != -1) {
        return USWAP_ERROR;
    }
    g_dev.uffd = uffd;
    return USWAP_SUCCESS;
}

static int get_uswap_uffd(void)
{
    return g_dev.uffd;
}

static void uswap_mutex_lock(void)
{
    pthread_mutex_lock(&g_dev.mutex);
}

static void uswap_mutex_unlock(void)
{
    pthread_mutex_unlock(&g_dev.mutex);
}

static void uswap_cond_wait(void)
{
    pthread_cond_wait(&g_dev.cond, &g_dev.mutex);
}

static void uswap_cond_wake(void)
{
    pthread_cond_signal(&g_dev.cond);
}

static int call_get_swapout_buf(const void *start_va, size_t len,
                                struct swap_data *swapout_data)
{
    return g_dev.ops->get_swapout_buf(start_va, len, swapout_data);
}

static int call_do_swapout(struct swap_data *swapout_data)
{
    return g_dev.ops->do_swapout(swapout_data);
}

static int call_do_swapin(const void *fault_addr, struct swap_data *swapin_data)
{
    return g_dev.ops->do_swapin(fault_addr, swapin_data);
}

static int call_release_buf(struct swap_data *swap_data)
{
    return g_dev.ops->release_buf(swap_data);
}

static int init_userfaultfd(void)
{
    struct uffdio_api uffdio_api;
    int uswap_uffd;
    int ret;

    uswap_uffd = syscall(__NR_userfaultfd, O_NONBLOCK);
    if (uswap_uffd < 0) {
        return USWAP_ERROR;
    }

    uffdio_api.api = UFFD_API;
    uffdio_api.features = 0;
    ret = ioctl(uswap_uffd, UFFDIO_API, &uffdio_api);
    if (ret < 0) {
        return ret;
    }
    ret = set_uswap_uffd(uswap_uffd);
    return ret;
}

int register_userfaultfd(void *addr, size_t size)
{
    struct uffdio_register uffdio_register;
    int ret;
    int uswap_uffd;
    size_t page_size;

    if (size > SSIZE_MAX || addr == NULL) {
        return USWAP_ERROR;
    }

    uswap_uffd = get_uswap_uffd();
    if (uswap_uffd < 0) {
        uswap_mutex_lock();
        ret = init_userfaultfd();
        uswap_cond_wake();
        uswap_mutex_unlock();
        if (ret == USWAP_ERROR) {
            uswap_log(USWAP_LOG_ERR, "init userfaultfd failed\n");
            return USWAP_ERROR;
        }
        uswap_uffd = get_uswap_uffd();
    }

    page_size = get_page_size();
    if (size >= page_size) {
        uffdio_register.range.start = (unsigned long)addr;
        uffdio_register.range.len = size;
        uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING |
                               UFFDIO_REGISTER_MODE_USWAP;
        ret = ioctl(uswap_uffd, UFFDIO_REGISTER, &uffdio_register);
        if (ret < 0) {
            uswap_log(USWAP_LOG_ERR, "register uffd failed\n");
            return USWAP_ERROR;
        }
        return USWAP_SUCCESS;
    }
    uswap_log(USWAP_LOG_ERR, "register uffd: the size smaller than page_size\n");
    return USWAP_ERROR;
}

int unregister_userfaultfd(void *addr, size_t size)
{
    struct uffdio_register uffdio_register;
    int uswap_uffd;
    int ret;

    uswap_uffd = get_uswap_uffd();
    if (uswap_uffd < 0 || size > SSIZE_MAX || addr == NULL) {
        return USWAP_ERROR;
    }

    uffdio_register.range.start = (unsigned long)addr;
    uffdio_register.range.len = size;
    ret = ioctl(uswap_uffd, UFFDIO_UNREGISTER, &uffdio_register);
    if (ret < 0) {
        uswap_log(USWAP_LOG_ERR, "unregister userfaultfd failed\n");
        return USWAP_ERROR;
    }
    return USWAP_SUCCESS;
}

int register_uswap(const char *name, size_t len,
                   const struct uswap_operations *ops)
{
    static struct uswap_operations uswap_ops;
    if (name == NULL || len > MAX_USWAP_NAME_LEN - 1) {
        return USWAP_ERROR;
    }

    if (ops == NULL) {
        return USWAP_ERROR;
    }

    if (ops->get_swapout_buf == NULL || ops->do_swapout == NULL ||
        ops->do_swapin == NULL || ops->release_buf == NULL) {
        return USWAP_ERROR;
    }

    uswap_ops = *ops;

    snprintf(g_dev.name, MAX_USWAP_NAME_LEN, "%s", name);
    g_dev.ops = &uswap_ops;
    g_dev.enabled = true;
    uswap_log(USWAP_LOG_INFO, "register uswap ops [%s] success\n", g_dev.name);
    return USWAP_SUCCESS;
}

static int mlock_pthread_stack(pthread_t tid)
{
    int ret;
    pthread_attr_t attr_t;
    void *stack_addr = NULL;
    size_t stack_size;

    ret = pthread_getattr_np(tid, &attr_t);
    if (ret < 0) {
        return USWAP_ERROR;
    }
    ret = pthread_attr_getstack(&attr_t, &stack_addr, &stack_size);
    pthread_attr_destroy(&attr_t);
    if (ret < 0) {
        return USWAP_ERROR;
    }

    ret = mlock(stack_addr, stack_size);
    if (ret < 0) {
        return USWAP_ERROR;
    }
    return USWAP_SUCCESS;
}

static int read_uffd_msg(int uffd, struct uffd_msg *msg)
{
    int ret;
    int msg_len = sizeof(struct uffd_msg);

    for (int i = 0; i < MAX_TRY_NUMS; i++) {
        ret = read(uffd, msg, msg_len);
        if (ret != msg_len) {
            if (errno == EAGAIN) {
                continue;
            }
            return USWAP_ERROR;
        }

        if (msg->event != UFFD_EVENT_PAGEFAULT) {
            uswap_log(USWAP_LOG_ERR, "unexpected event on userfaultfd\n");
            return USWAP_ERROR;
        }
        return USWAP_SUCCESS;
    }
    return USWAP_ABORT;
}

static int ioctl_uffd_copy_pages(int uffd, const struct swap_data *swapin_data)
{
    int ret;
    int offset = 0;
    size_t page_size = get_page_size();
    struct uffdio_copy uffdio_copy = {
        .len = page_size,
        .mode = 0,
        .copy = 0,
    };

    while (offset < swapin_data->len) {
        uffdio_copy.src = (unsigned long)swapin_data->buf + offset;
        uffdio_copy.dst = (unsigned long)swapin_data->start_va + offset;
        ret = ioctl(uffd, UFFDIO_COPY, &uffdio_copy);
        if (ret < 0 && errno != EEXIST) {
            uswap_log(USWAP_LOG_ERR, "uffd ioctl copy one page failed\n");
            return USWAP_ERROR;
        }
        offset += page_size;
    }

    return USWAP_SUCCESS;
}

static int ioctl_uffd_copy(int uffd, struct swap_data *swapin_data)
{
    int ret;
    struct uffdio_copy uffdio_copy = {
        .src = (unsigned long)swapin_data->buf,
        .dst = (unsigned long)swapin_data->start_va,
        .len = swapin_data->len,
        .mode = 0,
        .copy = 0,
    };
    for (int i = 0; i < MAX_TRY_NUMS; i++) {
        ret = ioctl(uffd, UFFDIO_COPY, &uffdio_copy);
        if (ret < 0) {
            if (errno == EAGAIN) {
                continue;
            }

            if (errno == EEXIST) {
                return USWAP_ALREADY_SWAPIN;
            }
            /*
            * 'start_va ~ start_va+len' may exceed the range of one vma.
            * If that is the case, copy one page at a time.
            */
            ret = ioctl_uffd_copy_pages(uffd, swapin_data);
            if (ret == USWAP_ERROR) {
                return USWAP_ERROR;
            }
        }
        return USWAP_SUCCESS;
    }
    uswap_log(USWAP_LOG_ERR, "ioctl copy max try failed\n");
    return USWAP_ERROR;
}

static void *swapin_thread(void *arg)
{
    struct swap_data swapin_data;
    struct uffd_msg uffd_msg;
    struct pollfd pollfd;
    unsigned long fault_addr;
    int uswap_uffd;
    int ret;

    prctl(PR_SET_NAME, "uswap-swapin", 0, 0, 0);

    uswap_mutex_lock();
    uswap_uffd = get_uswap_uffd();
    while (uswap_uffd < 0) {
        uswap_cond_wait();
        uswap_uffd = get_uswap_uffd();
    }
    uswap_mutex_unlock();
    while (1) {
        pollfd.fd = uswap_uffd;
        pollfd.events = POLLIN;
        ret = poll(&pollfd, 1, -1);
        if (ret < 0) {
            uswap_log(USWAP_LOG_ERR, "poll failed\n");
            usleep(10);
            continue;
        }
        ret = read_uffd_msg(uswap_uffd, &uffd_msg);
        if (ret == USWAP_ABORT) {
            continue;
        } else if (ret == USWAP_ERROR) {
            uswap_log(USWAP_LOG_ERR, "read uffd failed\n");
            continue;
        }

        fault_addr = uffd_msg.arg.pagefault.address;

        ret = call_do_swapin((void *)fault_addr, &swapin_data);
        if (ret == USWAP_ERROR) {
            uswap_log(USWAP_LOG_ERR, "do_swapin failed\n");
            exit(-1);
        }

        ret = ioctl_uffd_copy(uswap_uffd, &swapin_data);
        if (ret == USWAP_ERROR) {
            uswap_log(USWAP_LOG_ERR, "uffd ioctl copy failed\n");
            exit(-1);
        }
        ret = call_release_buf(&swapin_data);
        if (ret == USWAP_ERROR) {
            uswap_log(USWAP_LOG_ERR, "release buf failed\n");
        }
    }
}

static void* mmap_tmpva(const void *start, size_t len, int *is_dirty)
{
    unsigned long new_addr;
    new_addr = syscall(__NR_mmap, start, len, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_REPLACE, -1, 0);
    if ((void *)new_addr == MAP_FAILED) {
        if (errno != ENODEV) {
            uswap_log(USWAP_LOG_ERR, "the addr can't be swapout\n");
        }
        return MAP_FAILED;
    } else {
        *is_dirty = new_addr & MMAP_RETVAL_DIRTY_MASK;
        new_addr = new_addr & (~MMAP_RETVAL_DIRTY_MASK);
    }
    return (void *)new_addr;
}

static int do_swapout_once(const void *start, size_t len,
                           struct swap_data *swapout_data)
{
    int ret;
    int is_dirty = 1;
    void *tmpva = NULL;

    ret = call_get_swapout_buf(start, len, swapout_data);
    if (ret == USWAP_ALREADY_SWAPPED) {
        return USWAP_SUCCESS;
    }
    if (ret < 0) {
        uswap_log(USWAP_LOG_ERR, "get swapout buf error\n");
        return ret;
    }

    tmpva = mmap_tmpva(swapout_data->start_va, swapout_data->len, &is_dirty);

    swapout_data->flag = 0;
    if (tmpva != MAP_FAILED) {
        memcpy(swapout_data->buf, tmpva, swapout_data->len);
        munmap(tmpva, swapout_data->len);
    } else {
        swapout_data->flag |= USWAP_DATA_ABORT;
    }

    if (is_dirty != 0) {
        swapout_data->flag |= USWAP_DATA_DIRTY;
    }

    ret = call_do_swapout(swapout_data);

    return ret;
}

static int do_swapout(const void *start, size_t len)
{
    int ret;
    size_t succ_len = 0;
    struct swap_data swapout_data;

    while (succ_len < len) {
        ret = do_swapout_once(start + succ_len, len - succ_len, &swapout_data);
        if (ret < 0) {
            return ret;
        }
        if (succ_len >= len - swapout_data.len) {
            break;
        }
        succ_len += swapout_data.len;
    }
    uswap_log(USWAP_LOG_DEBUG, "do swapout addr: %p, len %lx\n", start, len);
    return succ_len;
}

static int vma_merge(const struct swap_vma *src, struct swap_vma *dst)
{
    int index = 0;
    int swapout_nums;
    size_t page_size = get_page_size();

    swapout_nums = src->length / sizeof(struct vma_addr);
    if (swapout_nums > MAX_VMA_NUM) {
        swapout_nums = MAX_VMA_NUM;
    }
    for (int i = 0; i < swapout_nums; i++) {
        if (src->vma_addrs[i].vma_len == 0 ||
            src->vma_addrs[i].vma_len > SSIZE_MAX) {
            continue;
        }
        if (src->vma_addrs[i].vma_len == page_size) {
            int j = i + 1;
            dst->vma_addrs[index].start_addr = src->vma_addrs[i].start_addr;
            dst->vma_addrs[index].vma_len = page_size;
            while (j < swapout_nums &&
                src->vma_addrs[j - 1].start_addr + page_size ==
                src->vma_addrs[j].start_addr) {
                j++;
                dst->vma_addrs[index].vma_len += page_size;
            }
            i = j - 1;
            index++;
        } else {
            dst->vma_addrs[index].start_addr = src->vma_addrs[i].start_addr;
            dst->vma_addrs[index].vma_len = src->vma_addrs[i].vma_len;
            index++;
        }
    }
    dst->length = index * sizeof(struct vma_addr);
    return USWAP_SUCCESS;
}

static int swapout_source_init(void)
{
    size_t page_size;
    int socket_fd = -1;

    page_size = get_page_size();
    socket_fd = init_socket();
    if (socket_fd < 0) {
        uswap_log(USWAP_LOG_DEBUG, "init_socket failed err:%d\n", errno);
        return USWAP_ERROR;
    }
    return socket_fd;
}

static void *swapout_thread(void *arg)
{
    struct swap_vma src, dst;
    unsigned long start;
    size_t len;
    int swapout_nums;
    int succ_count = 0;
    int socket_fd, client_fd;
    int ret;

    prctl(PR_SET_NAME, "uswap-swapout", 0, 0, 0);
    ret = swapout_source_init();
    if (ret < 0) {
        uswap_log(USWAP_LOG_ERR, "swapout source init failed\n");
        return NULL;
    }
    socket_fd = ret;
    while (1) {
        client_fd = sock_handle_rec(socket_fd, &src);
        if (client_fd <= 0) {
            uswap_log(USWAP_LOG_DEBUG, "sock_handle_rec failed\n");
            continue;
        }

        vma_merge(&src, &dst);
        ret = USWAP_SUCCESS;
        swapout_nums = dst.length / sizeof(struct vma_addr);
        for (int i = 0; i < swapout_nums; i++) {
            len = dst.vma_addrs[i].vma_len;
            start = dst.vma_addrs[i].start_addr;
            succ_count = do_swapout((void *)start, len);
            if (succ_count < 0) {
                uswap_log(USWAP_LOG_ERR, "do swapout once failed\n");
                ret = USWAP_ERROR;
            }
        }
        sock_handle_respond(client_fd, ret);
    }
    close(socket_fd);
}

int force_swapout(const void *addr, size_t len)
{
    int ret;
    size_t page_size = get_page_size();
    if (!is_uswap_enabled() || !is_uswap_threads_alive()) {
        return USWAP_ERROR;
    }
    if (addr == NULL || len >= SSIZE_MAX) {
        return USWAP_ERROR;
    }

    ret = do_swapout(addr, len);
    return ret;
}

static int create_uswap_threads(int swapin_nums)
{
    int ret;
    pthread_t swapout_tid;
    pthread_t swapin_tid[MAX_SWAPIN_THREAD_NUMS];

    if (swapin_nums <= 0 || swapin_nums > MAX_SWAPIN_THREAD_NUMS) {
        return USWAP_ERROR;
    }

    ret = pthread_create(&swapout_tid, NULL, swapout_thread, NULL);
    if (ret < 0) {
        uswap_log(USWAP_LOG_ERR, "can't create swapout thread");
        return USWAP_ERROR;
    }
    ret = mlock_pthread_stack(swapout_tid);
    if (ret == USWAP_ERROR) {
        uswap_log(USWAP_LOG_ERR, "mlock swapout thread stack failed\n");
        pthread_cancel(swapout_tid);
        return USWAP_ERROR;
    }

    for (int i = 0; i < swapin_nums; i++) {
        ret = pthread_create(&swapin_tid[i], NULL, swapin_thread, NULL);
        if (ret < 0) {
            uswap_log(USWAP_LOG_ERR, "can't create swapin thread\n");
            pthread_cancel(swapout_tid);
            for (int j = 0; j < i; j++) {
                pthread_cancel(swapin_tid[j]);
            }
            return USWAP_ERROR;
        }
        ret = mlock_pthread_stack(swapin_tid[i]);
        if (ret == USWAP_ERROR) {
            uswap_log(USWAP_LOG_ERR, "mlock swapin thread stack failed\n");
            pthread_cancel(swapout_tid);
            for (int j = 0; j <= i; j++) {
                pthread_cancel(swapin_tid[j]);
            }
            return USWAP_ERROR;
        }
    }

    return USWAP_SUCCESS;
}

int set_uswap_log_level(int log_level)
{
    return uswap_log_level_init(log_level);
}

int uswap_init(int swapin_nums)
{
    int ret;

    if (!is_uswap_enabled() || is_uswap_threads_alive()) {
        return USWAP_ERROR;
    }

    ret = create_uswap_threads(swapin_nums);
    if (ret == USWAP_ERROR) {
        return USWAP_ERROR;
    }

    g_dev.alive = true;
    return USWAP_SUCCESS;
}
