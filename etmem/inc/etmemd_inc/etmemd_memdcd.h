/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: YangXin
 * Create: 2021-4-20
 * Description: This is a header file of the function declaration for memdcd engine..
 ******************************************************************************/


#ifndef ETMEMD_MEMDCD_H
#define ETMEMD_MEMDCD_H

#include "etmemd_engine.h"

#define MAX_SOCK_PATH_LENGTH 108

struct memdcd_params {
	    struct task_executor *executor;
	        char memdcd_socket[MAX_SOCK_PATH_LENGTH];
};

int fill_engine_type_memdcd(struct engine *eng, GKeyFile *config);

#endif
