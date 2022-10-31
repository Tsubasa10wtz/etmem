# /******************************************************************************
#  * Copyright (c) Huawei Technologies Co., Ltd. 2019-2022. All rights reserved.
#  * etmem is licensed under the Mulan PSL v2.
#  * You can use this software according to the terms and conditions of the Mulan PSL v2.
#  * You may obtain a copy of Mulan PSL v2 at:
#  *     http://license.coscl.org.cn/MulanPSL2
#  * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
#  * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
#  * PURPOSE.
#  * See the Mulan PSL v2 for more details.
#  * Author: liuyongqiang
#  * Create: 2022-09-25
#  * Description: CMakefileList for uswap_server_llt to compile
#  ******************************************************************************/

project(userswap)

INCLUDE_DIRECTORIES(../../include)
INCLUDE_DIRECTORIES(${GLIB2_INCLUDE_DIRS})

SET(EXE userswap_server_llt)

add_executable(${EXE} userswap_server_llt.c)

target_link_libraries(${EXE} cunit ${BUILD_DIR}/lib/libuswap.a pthread ${GLIB2_LIBRARIES})
