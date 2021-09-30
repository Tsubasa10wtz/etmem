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
 * Author: liruilin
 * Create: 2020-10-30
 * Description: print log.
 ******************************************************************************/
#ifndef MEMDCD_LOG_H__
#define MEMDCD_LOG_H__


#define ERROR_STR_MAX_LEN 256

enum _log_level {
    _LOG_ERROR = 0,
    _LOG_WARN,
    _LOG_INFO,
    _LOG_DEBUG,
};


int init_log_level(const char *log_level_str);
int set_log_level(const int level);
void memdcd_log(const int level, const char *, ...);

#endif

