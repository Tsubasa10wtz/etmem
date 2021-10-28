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
 * Author: geruijun
 * Create: 2021-10-28
 * Description: This is a header file of damon engine.
 ******************************************************************************/

#ifndef ETMEMD_DAMON_H
#define ETMEMD_DAMON_H

#include "etmemd_project.h"

int etmemd_start_damon(struct project *proj);
int etmemd_stop_damon(void);
int fill_engine_type_damon(struct engine *eng, GKeyFile *config);

#endif
