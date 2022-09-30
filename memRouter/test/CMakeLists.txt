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
#  * Description: CMakeList for memRouter Unit test
#  ******************************************************************************/

project(memRouter C)
set(CMAKE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_bin)
add_subdirectory(Unit)
