#!/bin/bash

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
#  * Author: shikemeng
#  * Create: 2021-11-29
#  * Description: test shell for etmem_cslide_ops_llt
#  ******************************************************************************/
node_pair=$1

est_huge_num=2

proj_config="../conf/conf_cslide/proj.config"

eng_config="../conf/conf_cslide/eng.config"
eng_config_bak="../conf/conf_cslide/eng.config.bak"

task_config="../conf/conf_cslide/task.config"
task_config_bak="../conf/conf_cslide/task.config.bak"

# start server
./bin/etmemd -l 0 -s dt_socket &
sleep 1

# add project
cp ${eng_config} ${eng_config_bak}
./bin/etmem obj add -f ${proj_config} -s dt_socket
if [ "$?" != "0" ];then
    echo "add project fail"
    exit 1
fi

# add engine
sed -i "s/node_pair=2,0;3,1/node_pair=${node_pair}/g" ${eng_config_bak}
sed -i "s/node_mig_quota=256/node_mig_quota=0/g" ${eng_config_bak}
./bin/etmem obj add -f ${eng_config_bak} -s dt_socket
if [ "$?" != "0" ];then
    echo "add cslide engine fail"
    exit 1
fi

# add task with cold page
./bin/mem_access 0 $((test_huge_num * 2)) 0 &
mem_access0=$!
cp ${task_config} ${task_config_bak}
sed -i "s/value=1/value=${mem_access0}/g" ${task_config_bak}
sed -i "s/name=background1/name=mem_access0/g" ${task_config_bak}
./bin/etmem obj add -f ${task_config_bak} -s dt_socket
if [ "$?" != "0" ];then
    echo "add cslide engine fail"
    exit 1
fi

./bin/mem_access 0 $((test_huge_num * 2)) 1 &
mem_access1=$!
cp ${task_config} ${task_config_bak}
sed -i "s/value=1/value=${mem_access1}/g" ${task_config_bak}
sed -i "s/name=background1/name=mem_access1/g" ${task_config_bak}
./bin/etmem obj add -f ${task_config_bak} -s dt_socket
if [ "$?" != "0" ];then
    echo "add cslide engine fail"
    exit 1
fi

./bin/etmem project start -n test -s dt_socket
sleep 10
./bin/etmem engine showtaskpages -t mem_access0 -n test -e cslide -s dt_socket | grep -v pages | grep -v node > task0_pages

./bin/etmem engine showtaskpages -t mem_access1 -n test -e cslide -s dt_socket | grep -v pages | grep -v node > task1_pages

./bin/etmem engine showhostpages -n test -e cslide -s dt_socket | grep -v pages | grep -v node > host_pages

while read line; do
    node_pages=($line)
    if [ "${node_pages[0]}" == "0" ];then
        if [ "${node_pages[1]}" != "$((test_huge_num * 2 * 1024))" ];then
            echo "mem_access0 node ${node_pages[0]} used info wrong"
            exit 1
        fi
        if [ "${node_pages[2]}" != "0" ];then
            echo "mem_access0 node ${node_pages[0]} hot info wrong"
            exit 1
        fi
        if [ "${node_pages[3]}" != "$((test_huge_num * 2 * 1024))" ];then
            echo "mem_access0 node ${node_pages[0]} cold info wrong"
            exit 1
        fi
    else
        if [ "${node_pages[1]}" != "0" ];then
            echo "mem_access0 node ${node_pages[0]} used info wrong"
            exit 1
        fi
        if [ "${node_pages[2]}" != "0" ];then
            echo "mem_access0 node ${node_pages[0]} hot info wrong"
            exit 1
        fi
        if [ "${node_pages[3]}" != "0" ];then
            echo "mem_access0 node ${node_pages[0]} cold info wrong"
            exit 1
        fi
    fi
done < task0_pages

while read line; do
    node_pages=($line)
    if [ "${node_pages[0]}" == "0" ];then
        if [ "${node_pages[1]}" != "$((test_huge_num * 2 * 1024))" ];then
            echo "mem_access1 node ${node_pages[0]} used info wrong"
            exit 1
        fi
        if [ "${node_pages[2]}" != "$((test_huge_num * 2 * 1024))" ];then
            echo "mem_access1 node ${node_pages[0]} hot info wrong"
            exit 1
        fi
        if [ "${node_pages[3]}" != "0" ];then
            echo "mem_access1 node ${node_pages[0]} cold info wrong"
            exit 1
        fi
    else
        if [ "${node_pages[1]}" != "0" ];then
            echo "mem_access1 node ${node_pages[0]} used info wrong"
            exit 1
        fi
        if [ "${node_pages[2]}" != "0" ];then
            echo "mem_access1 node ${node_pages[0]} hot info wrong"
            exit 1
        fi
        if [ "${node_pages[3]}" != "0" ];then
            echo "mem_access1 node ${node_pages[0]} cold info wrong"
            exit 1
        fi
    fi
done < task1_pages

while read line; do
    node_pages=($line)
    hugepage_num=`cat /sys/devices/system/node/node${node_pages[0]}/hugepages/hugepages-2048kB/nr_hugepages`
    hugepage_kb=$((hugepage_num * 2 * 1024))
    if [ "${hugepage_kb}" != "${node_pages[1]}" ]; then
        echo "node ${node_pages[0]} total info wrong"
        exit 1
    fi
    if [ "${node_pages[0]}" == "0" ];then
        if [ "${node_pages[2]}" != "$((test_huge_num * 4 * 1024))" ];then
            echo "host node ${node_pages[0]} used info wrong"
            exit 1
        fi
        if [ "${node_pages[3]}" != "$((test_huge_num * 2 * 1024))" ];then
            echo "host node ${node_pages[0]} hot info wrong"
            exit 1
        fi
        if [ "${node_pages[4]}" != "$((test_huge_num * 2 * 1024))" ];then
            echo "host node ${node_pages[0]} cold info wrong"
            exit 1
        fi
    else
        if [ "${node_pages[2]}" != "0" ];then
            echo "host node ${node_pages[0]} used info wrong"
            exit 1
        fi
        if [ "${node_pages[3]}" != "0" ];then
            echo "host node ${node_pages[0]} hot info wrong"
            exit 1
        fi
        if [ "${node_pages[4]}" != "0" ];then
            echo "host node ${node_pages[0]} cold info wrong"
            exit 1
        fi
    fi
done < host_pages

./bin/etmem engine notsupport -t mem_access0 -n test -e cslide -s dt_socket
if [ "$?" == "0" ];then
    echo "engine notsupport cmd exe success unexpected"
    exit 1
fi

./bin/etmem engine showtaskpages -t noexisttask -n test -e cslide -s dt_socket
if [ "$?" == "0" ];then
    echo "showtaskpages for no exist task success unexpected"
    exit 1
fi

cp ${task_config} ${task_config_bak}
sed -i "s/name=background1/name=mem_access0/g" ${task_config_bak}
./bin/etmem obj del -f ${task_config_bak} -s dt_socket
if [ "$?" != "0" ];then
    echo "remove task mem_access0 fail"
    exit 1
fi

cp ${task_config} ${task_config_bak}
sed -i "s/name=background1/name=mem_access1/g" ${task_config_bak}
./bin/etmem obj del -f ${task_config_bak} -s dt_socket
if [ "$?" != "0" ];then
    echo "remove task mem_access1 fail"
    exit 1
fi
sleep 10

./bin/etmem engine showhostpages -n test -e cslide -s dt_socket
if [ "$?" == "0" ];then
    echo "exec cmd for engine without task success unexpected"
    exit 1
fi

killall mem_access
killall etmemd
exit 0
