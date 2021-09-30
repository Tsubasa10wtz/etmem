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
 * Description: This is a header file of etmemd log API.
 ******************************************************************************/

#ifndef ETMEMD_LOG_H
#define ETMEMD_LOG_H

#include <stdarg.h>

enum log_level {
    ETMEMD_LOG_DEBUG = 0,
    ETMEMD_LOG_INFO,
    ETMEMD_LOG_WARN,
    ETMEMD_LOG_ERR,
    ETMEMD_LOG_INVAL,
};

/*
 *  * function: init log level of print for etmemd.
 *
 *  in:     int log_level   - log level of etmemd to print
 *
 *  out:    0   - successed to init log level
 *          -1  - invalid level of log
 *  */
int etmemd_init_log_level(int log_level);

/*
 *  * function: interface of log in etmemd.
 *
 *  in:     enum log_level log_level  - the level of string passed in to log
 *          const char *format,         - the string logged
 *          ...                         - args passed in to log
 *  */
void etmemd_log(enum log_level log_level, const char *format, ...);
#endif
