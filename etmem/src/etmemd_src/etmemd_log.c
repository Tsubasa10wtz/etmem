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
 * Description: Etmemd log API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#include "etmemd_log.h"

enum log_level g_log_level = ETMEMD_LOG_ERR;

static void log_level_usage(void)
{
    printf("\n"
           "[0] for debug level\n"
           "[1] for info level\n"
           "[2] for warning level\n"
           "[3] for error level\n"
           "default level is error\n"
           "\n");
}

int etmemd_init_log_level(int log_level)
{
    if (log_level < 0 || log_level >= (int)ETMEMD_LOG_INVAL) {
        printf("error: invalid log level [%d]\n", log_level);
        log_level_usage();
        return -1;
    }

    g_log_level = (enum log_level)log_level;
    return 0;
}

void etmemd_log(enum log_level log_level, const char *format, ...)
{
    va_list args_in;

    /* do not log if log level is below the global level setted before */
    if (log_level < g_log_level) {
        return;
    }

    va_start(args_in, format);

    openlog("[etmemd] ", LOG_PID, LOG_USER);
    switch (log_level) {
        case ETMEMD_LOG_DEBUG:
            vsyslog(LOG_DEBUG, format, args_in);
            break;
        case ETMEMD_LOG_INFO:
            vsyslog(LOG_INFO, format, args_in);
            break;
        case ETMEMD_LOG_WARN:
            vsyslog(LOG_WARNING, format, args_in);
            break;
        case ETMEMD_LOG_ERR:
            vsyslog(LOG_ERR, format, args_in);
            break;
        default:
            va_end(args_in);
            printf("log_level is invalid, please check!\n");
            closelog();
            return;
    }

    va_end(args_in);
    closelog();
    return;
}

