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
 * Description: userswap interface
 ******************************************************************************/

#ifndef __USWAP_API_H__
#define __USWAP_API_H__

#include <sys/types.h>

#define USWAP_SUCCESS 			0
#define USWAP_ERROR 			(-1)
#define USWAP_UNREGISTER_MEM 	(-2)
#define USWAP_ALREADY_SWAPPED	(-3)
#define USWAP_ABORT				(-4)
#define USWAP_ALREADY_SWAPIN	(-5)

#define MAX_USWAP_NAME_LEN 32
#define MAX_SWAPIN_THREAD_NUMS 5

/* flag field of struct swap_data */
#define USWAP_DATA_DIRTY	0x1
#define USWAP_DATA_ABORT	0x2

struct swap_data {
    void *start_va;
    size_t len;
    void *buf;
    /*
     * Bit 0 (Dirty Flag):
     *   indicate the data in the range of 'start_va ~ start_va+len' is
     *   dirty if this bit is set.
     * Bit 1 (Abort Flag):
     *   This bit only takes affect in do_swapout. It indicates
     *   aborting the swapout operation if it is set.
     */
    size_t flag;
};

struct uswap_operations {
    int (*get_swapout_buf) (const void *, size_t, struct swap_data *);
    int (*do_swapout) (struct swap_data *);
    int (*do_swapin) (const void *, struct swap_data *);
    int (*release_buf) (struct swap_data *);
};

int set_uswap_log_level(int log_level);

int register_userfaultfd(void *addr, size_t size);

int unregister_userfaultfd(void *addr, size_t size);

int register_uswap(const char *name, size_t len,
                   const struct uswap_operations *ops);

int force_swapout(const void *addr, size_t len);

int uswap_init(int swapin_nums);
#endif
