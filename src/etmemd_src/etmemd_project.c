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
 * Description: Memigd project API.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#include "securec.h"
#include "etmemd_project.h"
#include "etmemd_engine.h"
#include "etmemd_common.h"
#include "etmemd_file.h"
#include "etmemd_log.h"

static SLIST_HEAD(project_list, project) g_projects = SLIST_HEAD_INITIALIZER(g_projects);

static void free_before_delete_project(struct project *proj)
{
    struct task *tmp_tk = NULL;

    etmemd_safe_free((void **)&proj->name);

    while (proj->tasks != NULL) {
        tmp_tk = proj->tasks;
        proj->tasks = proj->tasks->next;

        etmemd_free_task_pids(tmp_tk);
        etmemd_free_task_struct(&tmp_tk);
    }
}

static FILE *memid_project_open_conf(const char *file_name)
{
    FILE *file = NULL;
    char path[PATH_MAX] = {0};
    struct stat info;
    int r;
    int fd;

    if (realpath(file_name, path) == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "%s should be real path.\n", file_name);
        return NULL;
    }

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }

    r = fstat(fd, &info);
    if (r == -1) {
        close(fd);
        return NULL;
    }

    if (S_ISDIR(info.st_mode)) {
        etmemd_log(ETMEMD_LOG_ERR, "%s should be a file , not a folder.\n", path);
        close(fd);
        return NULL;
    }

    file = fdopen(fd, "r");

    return file;
}

static struct project *etmemd_project_init_struct(const char *project_name)
{
    struct project *proj = NULL;

    proj = (struct project *)calloc(1, sizeof(struct project));
    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for project fail.\n");
        return NULL;
    }

    proj->name = calloc(1, strlen(project_name) + 1);
    if (proj->name == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc project name fail.\n");
        free(proj);
        return NULL;
    }

    if (strncpy_s(proj->name, strlen(project_name) + 1, project_name,
                  strlen(project_name)) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "strncpy_s for project name fail\n");
        free(proj->name);
        proj->name = NULL;
        free(proj);
        return NULL;
    }

    return proj;
}

static struct project *get_proj_by_name(const char *name)
{
    struct project *proj = NULL;

    if (name == NULL || strlen(name) == 0) {
        etmemd_log(ETMEMD_LOG_ERR, "project name must be given and should not be empty.\n");
        return NULL;
    }

    SLIST_FOREACH(proj, &g_projects, entry) {
        if (strcmp(proj->name, name) == 0) {
            return proj;
        }
    }

    return NULL;
}

static enum opt_result check_add_params(const char *project_name, const char *file_name)
{
    struct project *proj = NULL;

    if (project_name == NULL || strlen(project_name) == 0) {
        etmemd_log(ETMEMD_LOG_ERR, "project name must be given and should not be empty.\n");
        return OPT_INVAL;
    }

    if (strlen(project_name) > PROJECT_NAME_MAX_LEN) {
        etmemd_log(ETMEMD_LOG_ERR, "the length of project name should not be larger than %d\n",
                   PROJECT_NAME_MAX_LEN);
        return OPT_INVAL;
    }

    if (file_name == NULL || strlen(file_name) == 0) {
        etmemd_log(ETMEMD_LOG_ERR, "file name must be given and should not be empty.\n");
        return OPT_INVAL;
    }

    if (strlen(file_name) > FILE_NAME_MAX_LEN) {
        etmemd_log(ETMEMD_LOG_ERR, "the length of file name should not be larger than %d\n",
                   FILE_NAME_MAX_LEN);
        return OPT_INVAL;
    }

    proj = get_proj_by_name(project_name);
    if (proj != NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "project %s already exist.\n", project_name);
        return OPT_PRO_EXISTED;
    }

    return OPT_SUCCESS;
}

enum opt_result etmemd_project_add(const char *project_name, const char *file_name)
{
    struct project *proj = NULL;
    FILE *file = NULL;
    enum opt_result ret;

    ret = check_add_params(project_name, file_name);
    if (ret != 0) {
        return ret;
    }

    file = memid_project_open_conf(file_name);
    if (file == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "open %s fail.\n", file_name);
        return OPT_INVAL;
    }

    proj = etmemd_project_init_struct(project_name);
    if (proj == NULL) {
        goto out_close;
    }

    if (etmemd_fill_proj_by_conf(proj, file) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get project from configuration file fail.\n");
        free_before_delete_project(proj);
        free(proj);
        proj = NULL;
        goto out_close;
    }

    SLIST_INSERT_HEAD(&g_projects, proj, entry);
    fclose(file);
    return OPT_SUCCESS;

out_close:
    fclose(file);
    return OPT_INVAL;
}

static void stop_tasks(struct project *proj)
{
    struct task *curr_task = NULL;

    if (!proj->start) {
        return;
    }

    proj->start = false;

    curr_task = proj->tasks;
    while (curr_task != NULL) {
        curr_task->stop_etmem(curr_task);
        etmemd_log(ETMEMD_LOG_DEBUG, "Stop for Task_value %s, project_name %s \n", curr_task->value,
                   curr_task->proj->name);
        curr_task = curr_task->next;
    }
}

enum opt_result etmemd_project_delete(const char *project_name)
{
    struct project *proj = NULL;

    proj = get_proj_by_name(project_name);
    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get project fail to delete.\n");
        return OPT_PRO_NOEXIST;
    }

    stop_tasks(proj);

    struct task *curr_task = NULL;
    curr_task = proj->tasks;
    while (curr_task != NULL) {
        if (curr_task->delete_etmem != NULL) {
            curr_task->delete_etmem(curr_task);
            curr_task = curr_task->next;
        }
    }

    SLIST_REMOVE(&g_projects, proj, project, entry);

    free_before_delete_project(proj);
    free(proj);

    return OPT_SUCCESS;
}

static void etmemd_project_print_line(void)
{
    int i;

    for (i = 0; i < PROJECT_SHOW_COLM_MAX; i++) {
        printf("-");
    }
    printf("\n");
}

static void etmemd_project_print(const struct project *proj)
{
    etmemd_project_print_line();
    printf("project: %s\n\n", proj->name);
    printf("%-8s %-32s %-32s %-32s %-16s\n",
           "number",
           "type",
           "value",
           "engine",
           "started");
    etmemd_print_tasks(proj->tasks);
    etmemd_project_print_line();
}

enum opt_result etmemd_project_show(void)
{
    struct project *proj = NULL;
    bool exists = false;

    SLIST_FOREACH(proj, &g_projects, entry) {
        etmemd_project_print(proj);
        exists = true;
    }

    if (!exists) {
        etmemd_project_print_line();
        printf("project: no project exits\n\n");
        etmemd_project_print_line();
    }

    if (fflush(stdout) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "fflush stdout fail.\n");
        return OPT_INTER_ERR;
    }

    return OPT_SUCCESS;
}

enum opt_result etmemd_migrate_start(const char *project_name)
{
    struct project *proj = NULL;
    struct task *curr_task = NULL;
    int start_task_num = 0;

    proj = get_proj_by_name(project_name);
    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get project %s fail.\n", project_name);
        return OPT_PRO_NOEXIST;
    }

    if (proj->start) {
        etmemd_log(ETMEMD_LOG_WARN, "project %s has been started already\n", proj->name);
        return OPT_PRO_STARTED;
    }

    proj->start = true;

    curr_task = proj->tasks;
    while (curr_task != NULL) {
        if (curr_task->start_etmem(curr_task) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "start task %s fail.\n",
                       curr_task->value);
            curr_task = curr_task->next;
            continue;
        }

        start_task_num++;
        curr_task = curr_task->next;
    }

    if (start_task_num == 0) {
        etmemd_log(ETMEMD_LOG_ERR, "all tasks start fail.\n");
        proj->start = false;
        return OPT_INTER_ERR;
    }

    return OPT_SUCCESS;
}

enum opt_result etmemd_migrate_stop(const char *project_name)
{
    struct project *proj = NULL;

    proj = get_proj_by_name(project_name);
    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get project fail.\n");
        return OPT_PRO_NOEXIST;
    }

    if (!proj->start) {
        etmemd_log(ETMEMD_LOG_WARN, "project %s has been stopped already or not start yet\n",
                   proj->name);
        return OPT_PRO_STOPPED;
    }
    stop_tasks(proj);
    proj->start = false;

    return OPT_SUCCESS;
}

void etmemd_stop_all_projects(void)
{
    struct project *proj = NULL;

    while (!SLIST_EMPTY(&g_projects)) {
        proj = SLIST_FIRST(&g_projects);
        SLIST_REMOVE_HEAD(&g_projects, entry);
        stop_tasks(proj);
        free_before_delete_project(proj);
        free(proj);
        proj = NULL;
    }
}
