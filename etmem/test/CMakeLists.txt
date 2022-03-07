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
#  * Author: louhongxiang
#  * Create: 2021-11-19
#  * Description: CMakefileList for etmem to compile
#  ******************************************************************************/

cmake_minimum_required (VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
project(etmem)

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "Architecture: ${ARCHITECTURE}" )

set(ETMEMD_SRC_DIR ${PROJECT_SOURCE_DIR}/../src/etmemd_src)
set(ETMEM_SRC_DIR ${PROJECT_SOURCE_DIR}/../src/etmem_src)
set(TEST_COMMON_DIR ${PROJECT_SOURCE_DIR}/common)
set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)

set(ETMEMD_SRC
 ${ETMEMD_SRC_DIR}/etmemd.c
 ${ETMEMD_SRC_DIR}/etmemd_file.c
 ${ETMEMD_SRC_DIR}/etmemd_common.c
 ${ETMEMD_SRC_DIR}/etmemd_rpc.c
 ${ETMEMD_SRC_DIR}/etmemd_log.c
 ${ETMEMD_SRC_DIR}/etmemd_project.c
 ${ETMEMD_SRC_DIR}/etmemd_engine.c
 ${ETMEMD_SRC_DIR}/etmemd_memdcd.c
 ${ETMEMD_SRC_DIR}/etmemd_slide.c
 ${ETMEMD_SRC_DIR}/etmemd_cslide.c
 ${ETMEMD_SRC_DIR}/etmemd_thirdparty.c
 ${ETMEMD_SRC_DIR}/etmemd_task.c
 ${ETMEMD_SRC_DIR}/etmemd_scan.c
 ${ETMEMD_SRC_DIR}/etmemd_threadpool.c
 ${ETMEMD_SRC_DIR}/etmemd_threadtimer.c
 ${ETMEMD_SRC_DIR}/etmemd_pool_adapter.c
 ${ETMEMD_SRC_DIR}/etmemd_migrate.c
 ${ETMEMD_SRC_DIR}/etmemd_damon.c)

set(ETMEM_SRC
 ${ETMEM_SRC_DIR}/etmem.c
 ${ETMEM_SRC_DIR}/etmem_project.c
 ${ETMEM_SRC_DIR}/etmem_obj.c
 ${ETMEM_SRC_DIR}/etmem_engine.c
 ${ETMEM_SRC_DIR}/etmem_rpc.c
 ${ETMEM_SRC_DIR}/etmem_common.c)

set(ETMEMD_CSLIDE_DEP
 ${ETMEMD_SRC_DIR}/etmemd.c
 ${ETMEMD_SRC_DIR}/etmemd_file.c
 ${ETMEMD_SRC_DIR}/etmemd_common.c
 ${ETMEMD_SRC_DIR}/etmemd_rpc.c
 ${ETMEMD_SRC_DIR}/etmemd_log.c
 ${ETMEMD_SRC_DIR}/etmemd_project.c
 ${ETMEMD_SRC_DIR}/etmemd_engine.c
 ${ETMEMD_SRC_DIR}/etmemd_memdcd.c
 ${ETMEMD_SRC_DIR}/etmemd_slide.c
 ${ETMEMD_SRC_DIR}/etmemd_thirdparty.c
 ${ETMEMD_SRC_DIR}/etmemd_task.c
 ${ETMEMD_SRC_DIR}/etmemd_scan.c
 ${ETMEMD_SRC_DIR}/etmemd_threadpool.c
 ${ETMEMD_SRC_DIR}/etmemd_threadtimer.c
 ${ETMEMD_SRC_DIR}/etmemd_pool_adapter.c
 ${ETMEMD_SRC_DIR}/etmemd_migrate.c
 ${ETMEMD_SRC_DIR}/etmemd_damon.c)

set(TEST_COMMON_SRC
 ${TEST_COMMON_DIR}/test_common.c)

add_library(
etmemd_shared
SHARED
${ETMEMD_SRC})

add_library(
etmem_shared
SHARED
${ETMEM_SRC})

add_library(
test_common
SHARED
${TEST_COMMON_SRC})

add_library(
cslide_dep
SHARED
${ETMEMD_CSLIDE_DEP})

add_executable(etmemd
 ${ETMEMD_SRC})

add_executable(etmem
 ${ETMEM_SRC})

set(EXECUTABLE_OUTPUT_PATH ${BUILD_DIR}/bin)

set(LIBRARY_OUTPUT_PATH ${BUILD_DIR}/lib)

include(FindPkgConfig)
pkg_search_module(GLIB2 REQUIRED glib-2.0)

target_include_directories(etmemd PRIVATE
 ${PROJECT_SOURCE_DIR}/../inc/etmemd_inc
 ${GLIB2_INCLUDE_DIRS})

target_include_directories(etmemd_shared PRIVATE
 ${PROJECT_SOURCE_DIR}/../inc/etmemd_inc
 ${GLIB2_INCLUDE_DIRS})

target_include_directories(etmem PRIVATE
 ${PROJECT_SOURCE_DIR}/../inc/etmem_inc)

target_include_directories(etmem_shared PRIVATE
 ${PROJECT_SOURCE_DIR}/../inc/etmem_inc)

target_include_directories(test_common PRIVATE
 ${PROJECT_SOURCE_DIR}/../inc/etmemd_inc
 ${PROJECT_SOURCE_DIR}/COMMON
 ${GLIB2_INCLUDE_DIRS})

target_include_directories(cslide_dep PRIVATE
 ${PROJECT_SOURCE_DIR}/../inc/etmemd_inc
 ${GLIB2_INCLUDE_DIRS})

if(COVERAGE_ENABLE)
 message(STATUS "Enable coverage compile option")
 SET(COVERAGE_OPTION "${COVERAGE_OPTION} -fprofile-arcs -ftest-coverage")
endif(COVERAGE_ENABLE)

if(ASAN_ENABLE)
 message(STATUS "Enable asan compile option")
 SET(ASAN_OPTIONS "${ASAN_OPTION} -fsanitize=address -fsanitize-recover=address")
endif(ASAN_ENABLE)

IF(CMAKE_BUILD_TYPE STREQUAL Debug)
 SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${COVERAGE_OPTION} ${ASAN_OPTIONS}")
 SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COVERAGE_OPTION} ${ASAN_OPTIONS}")
ENDIF()

# set common compile options
set(COMMON_COMPILE_OPT -fsigned-char -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -fno-strict-aliasing -Werror -Wformat -Wformat-security -D_GNU_SOURCE -fPIE -pie -fPIC -fstack-protector-strong -fno-common -DNDEBUG -O2 -D_FORTIFY_SOURCE=2 -Wall -Werror -Wl,-z,relro,-z,now,-z,noexecstack -Wtrampolines -pthread -Wno-pointer-sign -Wstrict-prototypes -Wold-style-definition -std=gnu99)

if (CONFIG_DEBUG STREQUAL "y")
 SET(COMMON_COMPILE_OPT ${COMMON_COMPILE_OPT} -g)
endif()

if( ${ARCHITECTURE} STREQUAL "aarch64")
 SET(COMMON_COMPILE_OPT ${COMMON_COMPILE_OPT} -march=armv8-a)
else()
 SET(COMMON_COMPILE_OPT ${COMMON_COMPILE_OPT} -march=core-avx-i -m64)
endif()

# set common share library compile options
set(SHARE_COMPILE_OPT -ldl -rdynamic)

# set common link options
set(COMMON_LINK_OPT "-fPIE -pie -fPIC -Wl,-z,relro,-z,now,-z,noexecstack -Wtrampolines")
set(COMMON_LINK_LIB pthread dl rt boundscheck)
set(SERVER_LINK_LIB numa ${GLIB2_LIBRARIES})

# target etmemd
target_compile_options(etmemd PRIVATE ${COMMON_COMPILE_OPT})
set_target_properties(etmemd PROPERTIES LINK_FLAGS ${COMMON_LINK_OPT})
target_link_libraries(etmemd PRIVATE ${COMMON_LINK_LIB} ${SERVER_LINK_LIB})

# target etmem
target_compile_options(etmem PRIVATE ${COMMON_COMPILE_OPT})
set_target_properties(etmem PROPERTIES LINK_FLAGS ${COMMON_LINK_OPT})
target_link_libraries(etmem PRIVATE ${COMMON_LINK_LIB} ${SERVER_LINK_LIB})

# target etmemd share
target_compile_options(etmemd_shared PRIVATE ${COMMON_COMPILE_OPT} ${SHARE_COMPILE_OPT})
set_target_properties(etmemd_shared PROPERTIES LINK_FLAGS ${COMMON_LINK_OPT})
target_link_libraries(etmemd_shared PRIVATE ${COMMON_LINK_LIB} ${SERVER_LINK_LIB})
SET_TARGET_PROPERTIES(etmemd_shared PROPERTIES OUTPUT_NAME "etmemd")

# target etmem share
target_compile_options(etmem_shared PRIVATE ${COMMON_COMPILE_OPT} ${SHARE_COMPILE_OPT})
set_target_properties(etmem_shared PROPERTIES LINK_FLAGS ${COMMON_LINK_OPT})
target_link_libraries(etmem_shared PRIVATE ${COMMON_LINK_LIB})
SET_TARGET_PROPERTIES(etmem_shared PROPERTIES OUTPUT_NAME "etmem")

# target test common share
target_compile_options(test_common PRIVATE ${COMMON_COMPILE_OPT} ${SHARE_COMPILE_OPT})
set_target_properties(test_common PROPERTIES LINK_FLAGS ${COMMON_LINK_OPT})
target_link_libraries(test_common PRIVATE ${COMMON_LINK_LIB} ${SERVER_LINK_LIB})
SET_TARGET_PROPERTIES(test_common PROPERTIES OUTPUT_NAME "test")

# target etmemd cslide share
target_compile_options(cslide_dep PRIVATE ${COMMON_COMPILE_OPT} ${SHARE_COMPILE_OPT})
set_target_properties(cslide_dep PROPERTIES LINK_FLAGS ${COMMON_LINK_OPT})
target_link_libraries(cslide_dep PRIVATE ${COMMON_LINK_LIB} ${SERVER_LINK_LIB})

add_subdirectory(etmem_common_func_llt_test)
add_subdirectory(etmem_log_ops_llt_test)
add_subdirectory(etmem_migrate_ops_llt_test)
add_subdirectory(etmem_task_ops_llt_test)
add_subdirectory(etmem_threadpool_ops_llt_test)
add_subdirectory(etmem_socket_ops_llt_test)
add_subdirectory(etmem_scan_ops_llt_test)
add_subdirectory(etmem_scan_ops_export_llt_test)
add_subdirectory(etmem_slide_ops_llt_test)
add_subdirectory(etmem_timer_ops_llt_test)
add_subdirectory(etmem_project_ops_llt_test)
add_subdirectory(etmem_cslide_ops_llt_test)
add_subdirectory(etmem_thirdparty_ops_llt_test)
