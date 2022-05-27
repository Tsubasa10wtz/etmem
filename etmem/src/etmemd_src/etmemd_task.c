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
 * Description: Etmemd task API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>

#include "securec.h"
#include "etmemd_log.h"
#include "etmemd_common.h"
#include "etmemd_task.h"
#include "etmemd_engine.h"
#include "etmemd_file.h"

static int get_pid_through_pipe(char *arg_pid[], const int *pipefd)
{
    pid_t pid;
    int status;
    int stdout_copy_fd;
    int ret;

    pid = fork();
    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        close(pipefd[0]);

        stdout_copy_fd = dup(STDOUT_FILENO);
        if (stdout_copy_fd < 0) {
            etmemd_log(ETMEMD_LOG_ERR, "dup(STDOUT_FILENO) fail.\n");
            close(pipefd[1]);
            return -1;
        }

        ret = dup2(pipefd[1], fileno(stdout));
        if (ret == -1) {
            etmemd_log(ETMEMD_LOG_ERR, "dup2 pipefd fail.\n");
            close(pipefd[1]);
            return -1;
        }

        if (execve(arg_pid[0], arg_pid, NULL) == -1) {
            etmemd_log(ETMEMD_LOG_ERR, "execve %s fail with %s.\n", arg_pid[0], strerror(errno));
            close(pipefd[1]);
            return -1;
        }

        if (fflush(stdout) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "fflush execve stdout fail.\n");
            close(pipefd[1]);
            return -1;
        }
        close(pipefd[1]);
        dup2(stdout_copy_fd, fileno(stdout));
    }

    /* wait for execve done */
    wait(&status);

    return 0;
}

void free_task_pid_mem(struct task_pid **tk_pid)
{
    struct engine *eng = (*tk_pid)->tk->eng;
    if (eng->ops->free_pid_params != NULL) {
        eng->ops->free_pid_params(eng, tk_pid);
    }
    etmemd_safe_free((void **)tk_pid);
}

static void clean_nouse_pid(struct task_pid **tk_pid)
{
    struct task_pid *tmp_pid = NULL;

    while (*tk_pid != NULL) {
        tmp_pid = (*tk_pid)->next;
        free_task_pid_mem(tk_pid);
        *tk_pid = tmp_pid;
    }
}

static struct task_pid *alloc_tkpid_node(unsigned int pid, struct task *tk)
{
    struct task_pid *tk_pid = NULL;
    struct engine *eng = tk->eng;

    tk_pid = (struct task_pid *)calloc(1, sizeof(struct task_pid));
    if (tk_pid == NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "malloc task pid fail.\n");
        return NULL;
    }
    tk_pid->pid = pid;
    tk_pid->tk = tk;

    if (eng->ops->alloc_pid_params != NULL && eng->ops->alloc_pid_params(eng, &tk_pid) != 0) {
        free(tk_pid);
        return NULL;
    }

    return tk_pid;
}

static struct task_pid **insert_task_pids(unsigned int pid, struct task_pid **current_pid, struct task *tk)
{
    struct task_pid *tk_pid = NULL;
    tk_pid = alloc_tkpid_node(pid, tk);
    if (tk_pid == NULL) {
        return NULL;
    }
    tk_pid->next = *current_pid;
    *current_pid = tk_pid;
    return &((*current_pid)->next);
}

static struct task_pid **update_task_pids(unsigned int pid, struct task_pid **current_pid, struct task *tk)
{
    struct task_pid *tk_tmp = NULL;
    if (*current_pid == NULL) {
        return insert_task_pids(pid, current_pid, tk);
    }

    if (pid == (*current_pid)->pid) {
        return &((*current_pid)->next);
    }

    if (pid > (*current_pid)->pid) {
        tk_tmp = *current_pid;
        *current_pid = (*current_pid)->next;
        free_task_pid_mem(&tk_tmp);
        return update_task_pids(pid, current_pid, tk);
    }

    return insert_task_pids(pid, current_pid, tk);
}

static int fill_task_pid(struct task *tk, const char *val)
{
    int ret;
    unsigned int pid;
    struct task_pid *tk_pid = NULL;
    
    ret = get_unsigned_int_value(val, &pid);
    if (ret != 0) {
        return -1;
    }

    if (tk->pids != NULL && tk->pids->pid == pid)
        return 0;

    clean_nouse_pid(&(tk->pids));
    tk_pid = alloc_tkpid_node(pid, tk);
    if (tk_pid == NULL) {
        return -1;
    }

    tk->pids = tk_pid;

    return 0;
}

static int read_fill_child_pid_file(struct task *tk, FILE *file)
{
    struct task_pid **current_pid = &(tk->pids->next);
    char line[FILE_LINE_MAX_LEN] = {};
    unsigned int pid;
    int ret;

    while (fgets(line, FILE_LINE_MAX_LEN, file) != NULL) {
        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }

        ret = get_unsigned_int_value(line, &pid);
        if (ret != 0) {
            return ret;
        }

        current_pid = update_task_pids(pid, current_pid, tk);
        if (current_pid == NULL) {
            return -1;
        }
    }

    clean_nouse_pid(current_pid);
    return 0;
}

static int get_pid_from_type_name(char *val, char *pid)
{
    char *arg_pid[] = {"/usr/bin/pgrep", "-x", val, NULL};
    FILE *file = NULL;
    int ret = -1;
    int pipefd[PIPE_FD_LEN]; /* used for pipefd[PIPE_FD_LEN] communication to obtain the task PID */

    if (pipe(pipefd) == -1) {
        return -1;
    }

    if (get_pid_through_pipe(arg_pid, pipefd) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get pid through pipe failed.\n");
        close(pipefd[1]);
        goto err_out;
    }

    close(pipefd[1]);
    file = fdopen(pipefd[0], "r");
    if (file == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "fopen pipefd[0] fail.\n");
        goto err_out;
    }

    ret = 0;
    if (fgets(pid, PID_STR_MAX_LEN, file) == NULL) {
        ret = -1;
    } else if (strlen(pid) >= 1 && pid[strlen(pid) - 1] == '\n') {
        /* remove the last character of \n, otherwise it will cause error of access() */
        pid[strlen(pid) - 1] = '\0';
    }

    fclose(file);
    return ret;

err_out:
    close(pipefd[0]);
    return ret;
}

int get_pid_from_task_type(const struct task *tk, char *pid)
{
    if (strcmp(tk->type, "pid") == 0) {
        if (strncpy_s(pid, PID_STR_MAX_LEN, tk->value, strlen(tk->value)) != EOK) {
            etmemd_log(ETMEMD_LOG_WARN, "strncpy_s for %s fail.\n", tk->value);
            return -1;
        }

        return 0;
    }

    if (strcmp(tk->type, "name") == 0) {
        if (get_pid_from_type_name(tk->value, pid) != 0) {
            etmemd_log(ETMEMD_LOG_DEBUG, "get pid from task <%s: %s> fail.\n",
                       tk->type, tk->value);
            return -1;
        }

        return 0;
    }

    return -1;
}

static int fill_task_child_pid(struct task *tk, char *pid)
{
    char *arg_pid[] = {"/usr/bin/pgrep", "-P", pid, NULL};
    FILE *file = NULL;
    int ret;
    int pipefd[PIPE_FD_LEN]; /* used for pipefd[PIPE_FD_LEN] communication to obtain the task PID */

    if (pipe(pipefd) == -1) {
        return -1;
    }

    ret = get_pid_through_pipe(arg_pid, pipefd);
    if (ret != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get pid through pipe failed.\n");
        close(pipefd[1]);
        goto err_out;
    }

    close(pipefd[1]);
    file = fdopen(pipefd[0], "r");
    if (file == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "fopen pipefd file fail.\n");
        ret = -1;
        goto err_out;
    }

    ret = read_fill_child_pid_file(tk, file);
    if (ret != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get child pid through pipe file failed.\n");
    }

    fclose(file);
    return ret;

err_out:
    close(pipefd[0]);
    return ret;
}

static bool check_task_pid_exists(const char *pid)
{
    size_t file_str_size = strlen(PROC_PATH) + strlen(pid) + 1;
    char file[file_str_size];

    if (snprintf_s(file, file_str_size, file_str_size - 1, "%s%s", PROC_PATH, pid) == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf for pid(%s) path fail\n", pid);
        return false;
    }

    if (access(file, F_OK) != 0) {
        return false;
    }

    return true;
}

void etmemd_free_task_pids(struct task *tk)
{
    struct task_pid *tmp_pid = NULL;

    if (tk == NULL) {
        return;
    }

    while (tk->pids != NULL) {
        tmp_pid = tk->pids->next;
        free_task_pid_mem(&tk->pids);
        tk->pids = tmp_pid;
    }
}

int etmemd_get_task_pids(struct task *tk, bool recursive)
{
    char pid[PID_STR_MAX_LEN] = {0};

    /* get the pid of target first */
    if (get_pid_from_task_type(tk, pid) != 0) {
        return -1;
    }

    /* check the pid of target exists or not */
    if (!check_task_pid_exists(pid)) {
        etmemd_log(ETMEMD_LOG_DEBUG, "pid %s of task %s %s is not alive\n", pid, tk->type, tk->value);
        return -1;
    }

    /* first, insert the pid of task into the pids list */
    if (fill_task_pid(tk, pid) != 0) {
        etmemd_log(ETMEMD_LOG_WARN, "fill task pid fail\n");
        return -1;
    }
    if (!recursive) {
        return 0;
    }

    /* then fill the child pids according to the pid of task */
    if (fill_task_child_pid(tk, pid) != 0) {
        etmemd_free_task_pids(tk);
        etmemd_log(ETMEMD_LOG_WARN, "get task child pids fail\n");
        return -1;
    }

    return 0;
}

static void clear_task_struct(struct task *task)
{
    etmemd_safe_free((void **)&task->type);
    etmemd_safe_free((void **)&task->value);
    etmemd_safe_free((void **)&task->name);
}

void etmemd_free_task_struct(struct task **tk)
{
    struct task *task = NULL;

    if (*tk == NULL) {
        return;
    }

    task = *tk;
    clear_task_struct(task);
    free(task);
    *tk = NULL;
}

void etmemd_print_tasks(int fd, const struct task *tk, char *eng_name, bool started)
{
    const struct task *tmp = tk;
    int i = 1;

    while (tmp != NULL) {
        dprintf_all(fd, "%-8d %-8s %-16s %-16s %-16s %-8s\n",
                    i,
                    tmp->type,
                    tmp->value,
                    tmp->name,
                    eng_name,
                    started ? "true" : "false");

        tmp = tmp->next;
        i++;
    }
}

static int fill_task_name(void *obj, void *val)
{
    struct task *tk = (struct task *)obj;
    char *name = (char *)val;
    tk->name = name;
    return 0;
}

static int fill_task_type(void *obj, void *val)
{
    struct task *tk = (struct task *)obj;
    char *type = (char *)val;
    if (strcmp(val, "pid") != 0 && strcmp(val, "name") != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid task type, must be pid or name.\n");
        free(val);
        return -1;
    }

    tk->type = type;
    return 0;
}

static int fill_task_value(void *obj, void *val)
{
    struct task *tk = (struct task *)obj;
    char *value = (char *)val;

    if (check_str_valid(value) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid task value, please check.\n");
        free(val);
        return -1;
    }

    tk->value = value;
    return 0;
}

static int fill_task_threads(void *obj, void *val)
{
    struct task *tk = (struct task *)obj;
    int max_threads = parse_to_int(val);
    int core;

    if (max_threads <= 0) {
        etmemd_log(ETMEMD_LOG_WARN,
                   "Thread count is abnormal, set the default minimum of current thread count to 1\n");
        max_threads = 1;
    }

    core = get_nprocs();
    /*
     * For IO intensive bussinesses, max-threads is limited to 2N + 1 of the maximum number
     * of threads
     */
    if (max_threads > 2 * core + 1) {
        etmemd_log(ETMEMD_LOG_WARN,
                   "max-threads is limited to 2N+1 of the maximum number of threads\n");
        max_threads = 2 * core + 1;
    }

    tk->max_threads = max_threads;
    return 0;
}

static int fill_task_swap_flag(void *obj, void *val)
{
    struct task *tk = (struct task *)obj;
    char *swap_flag = (char *)val;

    if (strcmp(swap_flag, "yes") == 0) {
        tk->swap_flag = 1;
        free(val);
        return 0;
    }

    if (strcmp(swap_flag, "no") == 0) {
        tk->swap_flag = 0;
        free(val);
        return 0;
    }

    free(val);
    etmemd_log(ETMEMD_LOG_ERR, "swap_flag para is not valid.\n");
    return -1;
}

struct config_item g_task_config_items[] = {
    {"name", STR_VAL, fill_task_name, false},
    {"type", STR_VAL, fill_task_type, false},
    {"value", STR_VAL, fill_task_value, false},
    {"swap_flag", STR_VAL, fill_task_swap_flag, true},
    {"max_threads", INT_VAL, fill_task_threads, true},
};

static int task_fill_by_conf(GKeyFile *config, struct task *tk)
{
    if (parse_file_config(config, TASK_GROUP, g_task_config_items,
                          ARRAY_SIZE(g_task_config_items), (void *)tk) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "parse task config fail\n");
        clear_task_struct(tk);
        return -1;
    }
    return 0;
}

struct task *etmemd_add_task(GKeyFile *config)
{
    struct task *tk = calloc(1, sizeof(struct task));

    if (tk == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc task struct fail\n");
        return NULL;
    }

    /* set default count of the thread pool to 1 */
    tk->max_threads = 1;
    if (task_fill_by_conf(config, tk) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "fill task from configuration file fail.\n");
        free(tk);
        return NULL;
    }
    return tk;
}

void etmemd_remove_task(struct task *tk)
{
    clear_task_struct(tk);
    free(tk);
}
