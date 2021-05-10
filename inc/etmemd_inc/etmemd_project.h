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
 * Description: This is a header file of the function declaration for etmemd operation API.
 ******************************************************************************/

#ifndef ETMEMD_PROJECT_H
#define ETMEMD_PROJECT_H

#include "etmemd_task.h"
#include "etmemd_engine.h"
#include "etmemd_project_exp.h"

/* set the length of project name to 32 */
#define PROJECT_NAME_MAX_LEN  32
#define FILE_NAME_MAX_LEN 256
#define PROJECT_SHOW_COLM_MAX 128

enum opt_result {
    OPT_SUCCESS = 0,
    OPT_INVAL,
    OPT_PRO_EXISTED,
    OPT_PRO_STARTED,
    OPT_PRO_STOPPED,
    OPT_PRO_NOEXIST,
    OPT_ENG_EXISTED,
    OPT_ENG_NOEXIST,
    OPT_TASK_EXISTED,
    OPT_TASK_NOEXIST,
    OPT_INTER_ERR,
    OPT_RET_END,
};

/*
 * function: Add project to etmem server.
 *
 * in:  GKeyFile *config - loaded config file
 *
 * out: OPT_SUCCESS(0)   - successed to add project
 *      other value      - failed to add project
 * */
enum opt_result etmemd_project_add(GKeyFile *config);

/*
 * function: Remove given project.
 *
 * in:  GKeyFile *config - loaded config file
 *
 * out: OPT_SUCCESS(0)   - successed to delete project
 *      other value      - failed to delete project
 * */
enum opt_result etmemd_project_remove(GKeyFile *config);

/*
 * function: Show all the projects.
 *
 * in:  const char *project_name - name of the project to start migrate
 *      int fd                   - client socket to show projects info
 *
 * out: OPT_SUCCESS(0)  - successed to show all the projects
 *      other value     - failed to show any project
 * */
enum opt_result etmemd_project_show(const char *project_name, int fd);

 /*
  * function: Start migrate in given project.
  *
  * in:  const char *project_name - name of the project to start migrate
  *
  * out: OPT_SUCCESS(0)  - successed to start migrate
  *      other value     - failed to start migrate
  * */
enum opt_result etmemd_migrate_start(const char *project_name);

 /*
  * function: Stop migrate in given project.
  *
  * in:  const char *project_name - name of the project to stop migrate
  *
  * out: OPT_SUCCESS(0)  - successed to stop migrate
  *      other value     - failed to stop migrate
  * */
enum opt_result etmemd_migrate_stop(const char *project_name);

enum opt_result etmemd_project_mgt_engine(const char *project_name, const char *eng_name, char *cmd, char *task_name,
        int sock_fd);
enum opt_result etmemd_project_add_engine(GKeyFile *config);
enum opt_result etmemd_project_remove_engine(GKeyFile *config);
enum opt_result etmemd_project_add_task(GKeyFile *config);
enum opt_result etmemd_project_remove_task(GKeyFile *config);

void etmemd_stop_all_projects(void);

#endif
