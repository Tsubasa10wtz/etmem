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
 * Description: This is a header file of the function declaration for common functions.
 ******************************************************************************/

#ifndef ETMEMD_COMMON_H
#define ETMEMD_COMMON_H

#include <stdio.h>
#include <stdbool.h>

#define PROC_PATH "/proc/"
#define FILE_LINE_MAX_LEN 1024
#define KEY_VALUE_MAX_LEN 64
#define DECIMAL_RADIX 10
#define ETMEMD_MAX_PARAMETER_NUM 6

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

/* in some system the max length of pid may be larger than 5, so we use 10 herr */
#define PID_STR_MAX_LEN 10

#define PIPE_FD_LEN 2

/*
 * function: parse cmdline passed to etmemd server.
 *
 * in:      int argc        - count of parameters
 *          char *argv[]    - array string of parameters
 *
 * out:     bool *is_help   - whether it has option of help
 *
 * retrun:  0 - successed to parse
 *          1 - failed to parse cmdline
 * */
int etmemd_parse_cmdline(int argc, char *argv[], bool *is_help);

bool check_str_format(char endptr);
int get_int_value(const char *val, int *value);
int get_unsigned_int_value(const char *val, unsigned int *value);
void etmemd_safe_free(void **ptr);

FILE *etmemd_get_proc_file(const char *pid, const char *file, int flags, const char *mode);

int get_keyword_and_value(const char *str, char *key, char *val);

int dprintf_all(int fd, const char *format, ...);

#endif
