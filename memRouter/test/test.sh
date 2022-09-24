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
#  * Description: test script for memRouter Unit test
#  ******************************************************************************/
pwd=$(cd "$(dirname "$0")";pwd)
echo $pwd
mkdir -p $pwd/build
cd $pwd/build
cmake ..
make
cd ..
chown $USER $pwd/config/*
./build/test_bin/memdcd_cmd_llt
./build/test_bin/memdcd_daemon_llt
./build/test_bin/memdcd_log_llt
./build/test_bin/memdcd_migrate_llt
./build/test_bin/memdcd_policy_llt
./build/test_bin/memdcd_process_llt
./build/test_bin/memdcd_threshold_llt
