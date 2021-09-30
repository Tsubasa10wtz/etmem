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
 * Description: userswap log definition.
 ******************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include "uswap_log.h"

enum log_level g_log_level = USWAP_LOG_ERR;

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

int uswap_log_level_init(enum log_level level)
{
    if (level < 0 || level >= USWAP_LOG_INVAL) {
        printf("error: invalid log level [%d]\n", level);
        log_level_usage();
        return -EINVAL;
    }
    g_log_level = level;
    return 0;
}

void uswap_log(enum log_level level, const char *format, ...)
{
    va_list args;

    if (level < g_log_level) {
        return;
    }

    va_start(args, format);

    switch (level) {
        case USWAP_LOG_DEBUG:
            openlog("[uswap_debug] ", LOG_PID, LOG_USER);
            vsyslog(LOG_INFO, format, args);
            break;
        case USWAP_LOG_INFO:
            openlog("[uswap_info] ", LOG_PID, LOG_USER);
            vsyslog(LOG_INFO, format, args);
            break;
        case USWAP_LOG_WARN:
            openlog("[uswap_warn] ", LOG_PID, LOG_USER);
            vsyslog(LOG_WARNING, format, args);
            break;
        case USWAP_LOG_ERR:
            openlog("[uswap_error] ", LOG_PID, LOG_USER);
            vsyslog(LOG_ERR, format, args);
            break;
        default:
            openlog("[uswap_error] ", LOG_PID, LOG_USER);
            vsyslog(LOG_ERR, "invalid uswap_log_level\n", args);
    }

    va_end(args);
    closelog();
    return;
}
