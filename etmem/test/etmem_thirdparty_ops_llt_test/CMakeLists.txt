# /******************************************************************************
#  * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
#  * etmem is licensed under the Mulan PSL v2.
#  * You can use this software according to the terms and conditions of the Mulan PSL v2.
#  * You may obtain a copy of Mulan PSL v2 at:
#  *     http://license.coscl.org.cn/MulanPSL2
#  * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
#  * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
#  * PURPOSE.
#  * See the Mulan PSL v2 for more details.
#  * Author: liubo
#  * Create: 2021-12-10
#  * Description: CMakefileList for etmem_thirdparty_ops_llt to compile
#  ******************************************************************************/

project(etmem)

INCLUDE_DIRECTORIES(../../inc/etmem_inc)
INCLUDE_DIRECTORIES(../../inc/etmemd_inc)
INCLUDE_DIRECTORIES(../common)
INCLUDE_DIRECTORIES(${GLIB2_INCLUDE_DIRS})

SET(EXE etmem_thirdparty_ops_llt)
SET(LIB thirdparty_engine)

add_library(${LIB} SHARED my_engine.c)
add_executable(${EXE} etmem_thirdparty_ops_llt.c)

target_link_libraries(${EXE} ${LIB} ${LIBNULL} cunit ${BUILD_DIR}/lib/libetmemd.so 
                      ${BUILD_DIR}/lib/libtest.so pthread dl rt boundscheck numa ${GLIB2_LIBRARIES})
