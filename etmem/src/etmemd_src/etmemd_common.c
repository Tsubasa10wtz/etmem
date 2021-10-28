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
 * Description: Implementation of common functions.
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "securec.h"
#include "etmemd_common.h"
#include "etmemd_rpc.h"
#include "etmemd_log.h"

#define IDLE_SCAN_MAGIC         0x66
#define IDLE_SCAN_ADD_FLAGS     _IOW(IDLE_SCAN_MAGIC, 0x0, unsigned int)

static void usage(void)
{
    printf("\nusage of etmemd:\n"
           "    etmemd [options]\n"
           "\noptions:\n"
           "    -l|--log-level <log-level>  Log level\n"
           "    -s|--socket <sockect name>  Socket name to listen to\n"
           "    -m|--mode-systemctl         mode used to start(systemctl)\n"        
           "    -h|--help                   Show this message\n");
}

static int etmemd_parse_opts_valid(int opt, bool *is_help)
{
    int ret;
    int param;
    *is_help = false;
    switch (opt) {
        case 's':
            if (etmemd_sock_name_set()) {
                printf("error: parse sock parameter repeated\n");
                ret = -1;
                break;
            }
            ret = etmemd_parse_sock_name(optarg);
            break;
        case 'l':
            ret = get_int_value(optarg, &param);
            if (ret != 0) {
                return ret;
            }
            ret = etmemd_init_log_level(param);
            break;
        case 'h':
            ret = 0;
            *is_help = true;
            usage();
            break;
        case 'm':
            ret = etmemd_deal_systemctl();
            break;
        case '?':
            printf("error: parse parameters failed\n");
            /* fallthrough */
        default:
            ret = -1;
            usage();
            break;
    }

    return ret;
}

static int etmemd_parse_check_result(int params_cnt, int argc, const bool *is_help)
{
    if ((params_cnt == 0 && argc > 1) || /* 1 means cmd should only be "etmemd" */
        (*is_help && argc > 2) || /* 2 means cmd should be "etmemd -h" */
        (params_cnt > 0 && !(*is_help) &&
        argc - 1 > params_cnt * 2)) { /* 2 means each parameter should follow one value */
        printf("warn: useless parameters passed in\n");
        return -1;
    }

    if (!etmemd_sock_name_set() && !(*is_help)) {
        printf("error: socket name of rpc to listen must be provided\n");
        usage();
        return -1;
    }

    return 0;
}

int etmemd_parse_cmdline(int argc, char *argv[], bool *is_help)
{
    const char *op_str = "s:l:mh";
    int params_cnt = 0;
    int opt, ret;
    struct option long_options[] = {
        {"socket", required_argument, NULL, 's'},
        {"log-level", required_argument, NULL, 'l'},
        {"mode-systemctl", no_argument, NULL, 'm'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0},
    };

    if (argv == NULL || argc > ETMEMD_MAX_PARAMETER_NUM) {
        printf("error: invalid parameters!\n");
        return -1;
    }

    while ((opt = getopt_long(argc, argv, op_str, long_options, NULL)) != -1) {
        ret = etmemd_parse_opts_valid(opt, is_help);
        if (ret != 0) {
            return -1;
        }
        params_cnt++;
        if (*is_help) {
            break;
        }
    }

    if (etmemd_parse_check_result(params_cnt, argc, is_help) != 0) {
        etmemd_sock_name_free();
        return -1;
    }

    return 0;
}

bool check_str_format(char endptr)
{
    if (errno != 0 || endptr != '\0') {
        return true;
    }
    return false;
}

int get_int_value(const char *val, int *value)
{
    char *pos = NULL;
    long int value_tmp;

    errno = 0;
    value_tmp = strtol(val, &pos, DECIMAL_RADIX);
    if (check_str_format(pos[0])) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid value, must be type of int.\n");
        return -1;
    }

    if (value_tmp < INT_MIN || value_tmp > INT_MAX) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid value %ld, out of range [%d-%d]\n",
                   value_tmp, INT_MIN, INT_MAX);
        return -1;
    }
    *value = (int)value_tmp;

    return 0;
}

int get_unsigned_int_value(const char *val, unsigned int *value)
{
    char *pos = NULL;
    unsigned long int value_tmp;

    errno = 0;
    value_tmp = strtoul(val, &pos, DECIMAL_RADIX);
    if (check_str_format(pos[0])) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid value, must be type of unsigned int.\n");
        return -1;
    }

    if (value_tmp > UINT_MAX) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid value %ld, out of range [%d]\n",
                   value_tmp, UINT_MAX);
        return -1;
    }
    *value = (unsigned int)value_tmp;

    return 0;
}

int get_unsigned_long_value(const char *val, unsigned long *value)
{
    char *pos = NULL;
    unsigned long value_tmp;

    errno = 0;
    value_tmp = strtoul(val, &pos, DECIMAL_RADIX);
    if (check_str_format(pos[0])) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid value, must be type of unsigned long.\n");
        return -1;
    }

    *value = value_tmp;
    return 0;
}

void etmemd_safe_free(void **ptr)
{
    if (ptr == NULL || *ptr == NULL) {
        return;
    }

    free(*ptr);
    *ptr = NULL;
}

static int get_status_num(char *getline, char *value, size_t value_len)
{
    size_t len = strlen(getline);
    int start_cp_index = 0;
    int end_cp_index = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        if (isdigit(getline[i])) {
            start_cp_index = i;
            end_cp_index = start_cp_index;
            break;
        }
    }

    for (; i < len; i++) {
        if (!isdigit(getline[i])) {
            end_cp_index = i - 1;
            break;
        }
    }

    if (strncpy_s(value, value_len, getline + start_cp_index, end_cp_index - start_cp_index + 1) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "strncpy_s for result failed.\n");
        return -1;
    }

    return 0;
}

static char *etmemd_get_proc_file_str(const char *pid, const char *file)
{
    char *file_name = NULL;
    size_t file_str_size;

    file_str_size = strlen(PROC_PATH) + strlen(pid) + strlen(file) + 1;
    file_name = (char *)calloc(file_str_size, sizeof(char));
    if (file_name == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for %s path fail\n", file);
        return NULL;
    }

    if (snprintf_s(file_name, file_str_size, file_str_size - 1, 
                    "%s%s%s", PROC_PATH, pid, file) == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf for %s fail\n", file);
        free(file_name);
        return NULL;
    }

    return file_name;
}

FILE *etmemd_get_proc_file(const char *pid, const char *file, int flags, const char *mode)
{
    char *file_name = NULL;
    int fd = -1;
    FILE *fp = NULL;

    file_name = etmemd_get_proc_file_str(pid, file);
    if (file_name == NULL) {
        return NULL;
    }

    fp = fopen(file_name, mode);
    if (fp == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "open file %s fail\n", file_name);
        goto free_file_name;
    }

    fd = fileno(fp);
    if (fd < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get fd of file %s fail\n", file_name);
        fclose(fp);
        fp = NULL;
        goto free_file_name;
    }

    if (flags != 0 && ioctl(fd, IDLE_SCAN_ADD_FLAGS, &flags) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set idle flags for %s fail with %s\n", pid, strerror(errno));
        fclose(fp);
        fp = NULL;
        goto free_file_name;
    }

free_file_name:
    free(file_name);
    return fp;
}

static inline bool is_valid_char_for_value(const char *valid, char c)
{
    return strchr(valid, c) != NULL;
}

static int check_value_valid(const char *valid, char c)
{
    if (isalpha(c) != 0 || isdigit(c) != 0 || is_valid_char_for_value(valid, c)) {
        return 0;
    }

    return -1;
}

static void get_valid_conf_str(const char *str, unsigned long start_idx,
                               char *val, const char *valid_symbol,
                               unsigned long *end_idx)
{
    unsigned int end_val_idx = 0;
    size_t str_len = strlen(str);

    *end_idx = start_idx;
    while (*end_idx < str_len && end_val_idx < KEY_VALUE_MAX_LEN - 1) {
        if (check_value_valid(valid_symbol, str[*end_idx]) != 0) {
            break;
        }
        val[end_val_idx] = str[*end_idx];
        end_val_idx++;
        (*end_idx)++;
    }

    val[end_val_idx] = '\0';
}

static bool check_result_str_valid(const char *result)
{
    if (strlen(result) == 0) {
        return false;
    }
    return true;
}

static unsigned long skip_colon_to_parse(const char *str, unsigned long idx)
{
    size_t str_len = strlen(str);

    while (idx < str_len) {
        if (!is_valid_char_for_value(" :\t", str[idx])) {
            break;
        }
        idx += 1;
    }

    return idx;
}

/*
 * the caller should make sure that str is not NULL, and the content
 * of str must not contain ' ' or '\t' at the beginning and the end
 * */
int get_keyword_and_value(const char *str, char *key, char *val)
{
    unsigned long idx = 0;
    unsigned long end_idx;
    size_t val_len;

    get_valid_conf_str(str, idx, key, "%_", &end_idx);
    if (!check_result_str_valid(key)) {
        etmemd_log(ETMEMD_LOG_ERR, "get keyword of %s fail\n", str);
        return -1;
    }
    if (!is_valid_char_for_value(" :\t", str[end_idx])) {
        etmemd_log(ETMEMD_LOG_ERR, "%s contains invalid symbol in keyword\n", str);
        return -1;
    }
    etmemd_log(ETMEMD_LOG_DEBUG, "parse config get key: %s\n", key);

    /* skip the string contains ' ', '\t' and ':' between key and value */
    idx = skip_colon_to_parse(str, end_idx);
    val_len = strlen(str) - idx;

    get_valid_conf_str(str, idx, val, ".%/_-", &end_idx);
    if (!check_result_str_valid(val)) {
        etmemd_log(ETMEMD_LOG_ERR, "get value of %s fail\n", str);
        return -1;
    }
    if (strlen(val) != val_len) {
        etmemd_log(ETMEMD_LOG_ERR, "%s contains invalid symbol in value\n", str);
        return -1;
    }
    etmemd_log(ETMEMD_LOG_DEBUG, "parse config get value: %s\n", val);

    return 0;
}

static int write_all(int fd, const char *buf)
{
    ssize_t rest = strlen(buf);
    ssize_t send_size;

    while (rest > 0) {
        send_size = write(fd, buf, rest);
        if (send_size < 0) {
            return -1;
        }
        rest -= send_size;
    }
    return 0;
}

int dprintf_all(int fd, const char *format, ...)
{
    char line[FILE_LINE_MAX_LEN];
    int ret;
    va_list args_in;

    va_start(args_in, format);
    ret = vsprintf_s(line, FILE_LINE_MAX_LEN, format, args_in);
    if (ret > FILE_LINE_MAX_LEN) {
        etmemd_log(ETMEMD_LOG_ERR, "fprintf_all fail as truncated.\n");
        va_end(args_in);
        return -1;
    }

    ret = write_all(fd, line);
    if (ret < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "write_all fail.\n");
        va_end(args_in);
        return -1;
    }

    va_end(args_in);
    return 0;
}

int get_mem_from_proc_file(const char *pid, const char *file_name, unsigned long *data, const char *cmpstr)
{
    FILE *file = NULL;
    char value[KEY_VALUE_MAX_LEN] = {};
    char get_line[FILE_LINE_MAX_LEN] = {};
    unsigned long val;
    int ret = -1;

    file = etmemd_get_proc_file(pid, file_name, 0, "r");
    if (file == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "cannot open %s for pid %s\n", file_name, pid);
        return ret;
    }

    while (fgets(get_line, FILE_LINE_MAX_LEN - 1, file) != NULL) {
        if (strstr(get_line, cmpstr) == NULL) {
            continue;
        }

        if (get_status_num(get_line, value, KEY_VALUE_MAX_LEN) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "get mem from /proc/%s/%s fail\n", pid, file_name);
            break;
        }

        if (get_unsigned_long_value(value, &val) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "get value with strtoul fail.\n");
            break;
        }

        *data = val;
        ret = 0;
        break;
    }

    fclose(file);
    return ret;
}

unsigned long get_pagesize(void)
{
    long pagesize;

    pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "get pageszie fail,error: %d\n", errno);
        return -1;
    }

    return (unsigned long)pagesize;
}
