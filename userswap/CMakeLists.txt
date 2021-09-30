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
#  * Author: LiuYongqiang
#  * Create: 2020-11-06
#  * Description: CMakefileList for userswap to compile
#  ******************************************************************************/

cmake_minimum_required (VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})

project(libuswap)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)

set(USWAP_SRC
	${SRC_DIR}/lib_uswap.c
	${SRC_DIR}/uswap_server.c
	${SRC_DIR}/uswap_log.c)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(uswap STATIC ${USWAP_SRC})

target_compile_options(uswap PRIVATE -fPIC -ftrapv -D_FORTIFY_SOURCE=2 -O2
-fstack-protector-strong -Wl,-z,relro,-z,now,-z,noexecstack -fvisibility=hidden)

install(TARGETS uswap PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE DESTINATION /usr/lib64)
install(FILES ${PROJECT_SOURCE_DIR}/include/uswap_api.h DESTINATION /usr/include)
