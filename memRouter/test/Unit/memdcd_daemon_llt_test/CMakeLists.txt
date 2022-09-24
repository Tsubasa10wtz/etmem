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
#  * Description: CMakeList for Unit test of memdcd_daemon
#  ******************************************************************************/

project(memRouter C)

INCLUDE_DIRECTORIES(../../../include ../../stub/)
SET(EXE memdcd_daemon_llt)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_OUTPUT_DIRECTORY}/)
set(SRC_DIR ../../../src)

add_executable(${EXE} 
        ${EXE}.c
        ../../stub/alloc_memory.c
        ../../test_driver/test_driver.c
        ../../test_driver/memdcd_daemon.c
        ${SRC_DIR}/memdcd_process.c
        ${SRC_DIR}/memdcd_policy.c
        ${SRC_DIR}/memdcd_policy_threshold.c
        ${SRC_DIR}/memdcd_migrate.c
        ${SRC_DIR}/memdcd_process.c
        ${SRC_DIR}/memdcd_cmd.c
        ${SRC_DIR}/memdcd_log.c)

target_compile_definitions(${EXE} PRIVATE _GNU_SOURCE)

target_link_libraries(${EXE} cunit pthread dl rt numa json-c)

