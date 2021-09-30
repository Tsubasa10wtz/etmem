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
 * Description: This is a header file of the data structure definition for etmem configure and object.
 ******************************************************************************/

#ifndef __ETMEM_H__
#define __ETMEM_H__

#include <sys/queue.h>

typedef enum etmem_cmd_e {
    ETMEM_CMD_ADD,
    ETMEM_CMD_DEL,
    ETMEM_CMD_START,
    ETMEM_CMD_STOP,
    ETMEM_CMD_SHOW,
    ETMEM_CMD_ENGINE,
    ETMEM_CMD_HELP,
} etmem_cmd;

struct etmem_conf {
    char *obj;
    etmem_cmd cmd;
    int argc;
    char **argv;
};

struct etmem_obj {
    char *name;
    void (*help)(void);
    int (*do_cmd)(struct etmem_conf *conf);

    SLIST_ENTRY(etmem_obj) entry;
};

void etmem_register_obj(struct etmem_obj *obj);
void etmem_unregister_obj(const struct etmem_obj *obj);

#endif
