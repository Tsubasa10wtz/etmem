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
 * Create: 2021-4-30
 * Description: This is a header file of the function declaration for thirdparty function.
 ******************************************************************************/

#ifndef ETMEMD_THIRDPARTY_H
#define ETMEMD_THIRDPARTY_H

#include "etmemd_engine.h"

int fill_engine_type_thirdparty(struct engine *eng, GKeyFile *config);
void clear_engine_type_thirdparty(struct engine *eng);

#endif
