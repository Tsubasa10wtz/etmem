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
 * Description: This is a header file of the function declaration for adapter API.
 ******************************************************************************/

#ifndef ETMEMD_POOL_ADAPTER_H
#define ETMEMD_POOL_ADAPTER_H

#include "etmemd_threadpool.h"
#include "etmemd_threadtimer.h"
#include "etmemd_project.h"
#include "etmemd_log.h"

struct task_executor {
    struct task *tk;
    void *(*func)(void *arg);
};

/*
 * Start process scanning and memory migration
 * */
int start_threadpool_work(struct task_executor *executor);

/*
 * Stop process and cleanup resource of scanning and memory migration
 * */
void stop_and_delete_threadpool_work(struct task *tk);

#endif //ETMEMD_POOL_ADAPTER_H
