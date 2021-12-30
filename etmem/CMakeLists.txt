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
#  * Create: 2019-12-10
#  * Description: CMakefileList for etmem to compile
#  ******************************************************************************/

cmake_minimum_required (VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
project(etmem)

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "Architecture: ${ARCHITECTURE}" )

set(ETMEMD_SRC_DIR ${PROJECT_SOURCE_DIR}/src/etmemd_src)
set(ETMEM_SRC_DIR ${PROJECT_SOURCE_DIR}/src/etmem_src)
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

add_executable(etmemd
 ${ETMEMD_SRC})

add_executable(etmem
 ${ETMEM_SRC})

add_library(etmemd_scan SHARED
 ${ETMEMD_SRC})

set(EXECUTABLE_OUTPUT_PATH ${BUILD_DIR}/bin)

set(LIBRARY_OUTPUT_PATH ${BUILD_DIR}/lib)

include(FindPkgConfig)
pkg_search_module(GLIB2 REQUIRED glib-2.0)

target_include_directories(etmemd PRIVATE
 ${PROJECT_SOURCE_DIR}/inc/etmemd_inc
 ${GLIB2_INCLUDE_DIRS})

target_include_directories(etmem PRIVATE
 ${PROJECT_SOURCE_DIR}/inc/etmem_inc)

target_include_directories(etmemd_scan PRIVATE
 ${PROJECT_SOURCE_DIR}/inc/etmemd_inc
 ${GLIB2_INCLUDE_DIRS})

target_compile_options(etmemd PRIVATE -fsigned-char -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -fno-strict-aliasing -Werror -Wformat -Wformat-security -D_GNU_SOURCE -fPIE -pie -fPIC -fstack-protector-strong -fno-common -DNDEBUG -O2 -D_FORTIFY_SOURCE=2 -Wall -Werror -Wl,-z,relro,-z,now,-z,noexecstack -Wtrampolines -pthread -Wno-pointer-sign -Wstrict-prototypes -Wold-style-definition -std=gnu99)


if(CONFIG_DEBUG STREQUAL "y")
 target_compile_options(etmemd PRIVATE -g)
endif()

set_target_properties(etmemd PROPERTIES LINK_FLAGS "-s -fPIE -pie -fPIC -Wl,-z,relro,-z,now,-z,noexecstack -Wtrampolines")
target_link_libraries(etmemd PRIVATE pthread dl rt boundscheck numa ${GLIB2_LIBRARIES})

if( ${ARCHITECTURE} STREQUAL "aarch64" )
 target_compile_options(etmemd PRIVATE -march=armv8-a)
else()
 target_compile_options(etmemd PRIVATE -march=core-avx-i -m64)
endif()

target_compile_options(etmem PRIVATE -fsigned-char -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -fno-strict-aliasing -Werror -Wformat -Wformat-security -D_GNU_SOURCE -fPIE -pie -fPIC -fstack-protector-strong -fno-common -DNDEBUG -O2 -D_FORTIFY_SOURCE=2 -Wall -Werror -Wl,-z,relro,-z,now,-z,noexecstack -Wtrampolines -pthread -Wno-pointer-sign -Wstrict-prototypes -Wold-style-definition -std=gnu99)

if(CONFIG_DEBUG STREQUAL "y")
 target_compile_options(etmem PRIVATE -g)
endif()

set_target_properties(etmem PROPERTIES LINK_FLAGS "-s -fPIE -pie -fPIC -Wl,-z,relro,-z,now,-z,noexecstack -Wtrampolines")
target_link_libraries(etmem PRIVATE pthread dl rt boundscheck)

if( ${ARCHITECTURE} STREQUAL "aarch64" )
 target_compile_options(etmem PRIVATE -march=armv8-a)
else()
 target_compile_options(etmem PRIVATE -march=core-avx-i -m64)
endif()

target_compile_options(etmemd_scan PRIVATE -fsigned-char -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -fno-strict-aliasing -Werror -Wformat -Wformat-security -D_GNU_SOURCE -fPIE -pie -fPIC -fstack-protector-strong -fno-common -DNDEBUG -O2 -D_FORTIFY_SOURCE=2 -Wall -Werror -Wl,-z,relro,-z,now,-z,noexecstack -Wtrampolines -pthread -Wno-pointer-sign -Wstrict-prototypes -Wold-style-definition -std=gnu99 -fPIC -shared)


if(CONFIG_DEBUG STREQUAL "y")
 target_compile_options(etmemd_scan PRIVATE -g)
endif()

set_target_properties(etmemd_scan PROPERTIES LINK_FLAGS "-s -fPIE -pie -fPIC -Wl,-z,relro,-z,now,-z,noexecstack -Wtrampolines -Wl,--version-script=${ETMEMD_SRC_DIR}/etmemd_scan.version")
target_link_libraries(etmemd_scan PRIVATE pthread dl rt boundscheck numa ${GLIB2_LIBRARIES})

if( ${ARCHITECTURE} STREQUAL "aarch64" )
 target_compile_options(etmemd_scan PRIVATE -march=armv8-a)
else()
 target_compile_options(etmemd_scan PRIVATE -march=core-avx-i -m64)
endif()

install(TARGETS etmem etmemd 
        PERMISSIONS OWNER_READ OWNER_EXECUTE 
        RUNTIME DESTINATION /usr/bin)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/conf/ 
        DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE 
        DESTINATION /etc/etmem)
