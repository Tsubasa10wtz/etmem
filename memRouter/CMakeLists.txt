# /******************************************************************************
#  * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
#  * etmem is licensed under the Mulan PSL v2.
#  * You can use this software according to the terms and conditions of the Mulan PSL v2.
#  * You may obtain a copy of Mulan PSL v2 at:
#  *     http://license.coscl.org.cn/MulanPSL2
#  * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
#  * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
#  * PURPOSE.
#  * See the Mulan PSL v2 for more details.
#  * Author: YangXin
#  * Create: 2020-09-08
#  * Description: CMakefileList for memRouter to compile
#  ******************************************************************************/

cmake_minimum_required (VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
project(memdcd C)

set(CMAKE_C_STANDARD 99)

set(LOCAL_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

IF(CMAKE_BUILD_TYPE STREQUAL Test)
    IF(COVERAGE_ENABLE)
        MESSAGE(STATUS "Enable coverage compile option")
        SET(COVERAGE_OPTION "${COVERAGE_OPTION} -fprofile-arcs -ftest-coverage")
    ENDIF(COVERAGE_ENABLE)
    IF(ASAN_ENABLE)
        MESSAGE(STATUS "Enable asan compile option")
        SET(ASAN_OPTIONS "${ASAN_OPTION} -fsanitize=address -fsanitize-recover=address")
    ENDIF(ASAN_ENABLE)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG} ${COVERAGE_OPTION} ${ASAN_OPTIONS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} ${COVERAGE_OPTION} ${ASAN_OPTIONS}")
    add_subdirectory(test)
ENDIF()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)

set(MEMROUTER_SRC
        ${SRC_DIR}/memdcd.c
        ${SRC_DIR}/memdcd_policy.c
        ${SRC_DIR}/memdcd_policy_threshold.c
        ${SRC_DIR}/memdcd_migrate.c
        ${SRC_DIR}/memdcd_process.c
        ${SRC_DIR}/memdcd_daemon.c
        ${SRC_DIR}/memdcd_cmd.c
        ${SRC_DIR}/memdcd_log.c
        )

add_executable(memdcd ${MEMROUTER_SRC})

target_include_directories(memdcd  PRIVATE
	${INCLUDE_DIR}/)

add_definitions(-D_GNU_SOURCE -DNDEBUG -D_FORTIFY_SOURCE=2)
target_compile_options(memdcd PRIVATE -fsigned-char --param ssp-buffer-size=4 -fno-omit-frame-pointer -Wall -Wextra 
-Wno-missing-field-initializers -Wmissing-declarations -fno-strict-aliasing
-Wformat -Wformat-security -fPIE -pie -fPIC -ftrapv
-Wno-error=deprecated-declarations -fstack-protector-all -fno-common -O2 -Wall -Wl,-z,relro,-z,now,-z,noexecstack -Wtrampolines -pthread -Wstrict-prototypes -Wl,--no-whole-archive -Wold-style-definition -std=gnu99 -Wno-error=unused-result -Wno-error=strict-prototypes -Wno-error=old-style-definition -Wno-error=missing-declarations -Wno-error=format-overflow -I${INCLUDE_DIR})

if(CONFIG_DEBUG STREQUAL "y")
    target_compile_options(memdcd PRIVATE -g)
endif()

target_link_libraries(memdcd PRIVATE pthread dl rt numa json-c cap -s)

install(TARGETS memdcd
    PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
    RUNTIME DESTINATION /usr/bin
    )
