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
 * Description: userswap log headfile
 ******************************************************************************/

#ifndef __USWAP_LOG_H__
#define __USWAP_LOG_H__

enum log_level {
    USWAP_LOG_DEBUG = 0,
    USWAP_LOG_INFO,
    USWAP_LOG_WARN,
    USWAP_LOG_ERR,
    USWAP_LOG_INVAL,
};

int uswap_log_level_init(enum log_level level);

void uswap_log(enum log_level level, const char *format, ...);
#endif
