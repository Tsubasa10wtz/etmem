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
#  * Create: 2021-12-07
#  * Description: This is a shell script of the unit test for etmem commont functions
#  ******************************************************************************/

test_start_as_service()
{
    ./bin/etmemd -l 0 -s dt_socket -m &
    etmemd_pid=$!
    echo "etmemd_pid ${etmemd_pid}"
    sleep 1
    ./bin/etmem obj add -f ../conf/conf_slide/config_file -s dt_socket
    if [ "$?" != "0" ];then
        echo "add config fail to etmemd service unexpected"
        exit 1
    fi
    ps -aux |grep -v grep |grep "etmemd -l 0 -s dt_socket -m"
    if [ "$?" != "0" ];then
        echo "etmemd service exit unexpected after add config"
        exit 1
    fi
    killall etmemd
}

test_start_as_service
