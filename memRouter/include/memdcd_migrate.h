/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * etmem/memRouter licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: zhangxuzhou
 * Create: 2020-09-18
 * Description: engines migrating memory pages
 ******************************************************************************/
#ifndef MEMDCD_CONNECT_H
#define MEMDCD_CONNECT_H
#include "memdcd_process.h"

int send_to_userswap(int pid, const struct migrate_page_list *pages);

#endif /* MEMDCD_CONNECT_H */
