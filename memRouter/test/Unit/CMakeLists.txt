# /******************************************************************************
#  * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
#  * etmem is licensed under the Mulan PSL v2.
#  * You can use this software according to the terms and conditions of the Mulan PSL v2.
#  * You may obtain a copy of Mulan PSL v2 at:
#  *     http://license.coscl.org.cn/MulanPSL2
#  * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
#  * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
#  * PURPOSE.
#  * See the Mulan PSL v2 for more details.
#  * Author: yangxin
#  * Create: 2022-09-26
#  * Description: CMakeList for Unit test of memdcd
#  ******************************************************************************/

project(memRouter C)
MESSAGE( STATUS "this var key = ${CMAKE_SOURCE_DIR}.")
add_subdirectory(memdcd_cmd_llt_test)  
add_subdirectory(memdcd_daemon_llt_test)  
add_subdirectory(memdcd_log_llt_test)  
add_subdirectory(memdcd_migrate_llt_test)  
add_subdirectory(memdcd_process_llt_test)
add_subdirectory(memdcd_threshold_llt_test)  
add_subdirectory(memdcd_policy_llt_test)  
