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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include "memdcd_log.h"

static int log_level = 0;

int set_log_level(const int level)
{
    log_level = level;
    return log_level;
}

int init_log_level(const char *log_level_str)
{
    if (log_level_str == NULL)
        return set_log_level(_LOG_INFO);
    if (strcmp(log_level_str, "LOG_DEBUG") == 0)
        return set_log_level(_LOG_DEBUG);
    if (strcmp(log_level_str, "LOG_INFO") == 0)
        return set_log_level(_LOG_INFO);
    if (strcmp(log_level_str, "LOG_WARN") == 0)
        return set_log_level(_LOG_WARN);
    if (strcmp(log_level_str, "LOG_ERROR") == 0)
        return set_log_level(_LOG_ERROR);
    memdcd_log(_LOG_ERROR, "Error initint log_level: %s.", log_level_str);
    return -EINVAL;
}

void memdcd_log(const int level, const char *va_alist, ...)
{
    va_list ap;

    if (level > log_level)
        return;

    va_start(ap, va_alist);
    switch (level) {
        case _LOG_INFO:
            openlog("[MEMDCD_INFO] ", LOG_PID, LOG_USER);
            vsyslog(LOG_INFO, va_alist, ap);
            break;
        case _LOG_DEBUG:
            openlog("[MEMDCD_DEBUG] ", LOG_PID, LOG_USER);
            vsyslog(LOG_INFO, va_alist, ap);
            break;
        case _LOG_WARN:
            openlog("[MEMDCD_WARNING] ", LOG_PID, LOG_USER);
            vsyslog(LOG_WARNING, va_alist, ap);
            break;
        case _LOG_ERROR:
            openlog("[MEMDCD_ERROR] ", LOG_PID, LOG_USER);
            vsyslog(LOG_ERR, va_alist, ap);
            break;
        default:
            va_end(ap);
            return;
    }

    va_end(ap);
    closelog();
    return;
}