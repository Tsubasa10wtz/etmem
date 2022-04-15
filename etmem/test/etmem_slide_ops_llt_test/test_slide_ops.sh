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
#  * Author: liubo
#  * Create: 2021-11-29
#  * Description: test shell for etmem_slide_ops_llt
#  ******************************************************************************/

set +e

config_file="../conf/conf_slide/config_file"
config_file_bak="../conf/conf_slide/config_file_bak"
Etmem_slide_test="slide_test"
project_configfile=`pwd`/"slide_project_swap_test.yaml"
task_configfile=`pwd`/"slide_task_test.yaml"

function rand_file_Str
{
    j=0;
    for i in {a..z};do array[$j]=$i;j=$(($j+1));done
    for i in {A..Z};do array[$j]=$i;j=$(($j+1));done
    for ((i=0;i<257;i++));do filestrs="$filestrs${array[$(($RANDOM%$j))]}";done
}

function rand_pro_Str
{
    j=0;
    for i in {a..z};do array[$j]=$i;j=$(($j+1));done
    for i in {A..Z};do array[$j]=$i;j=$(($j+1));done
    for ((i=0;i<33;i++));do prostrs="$prostrs${array[$(($RANDOM%$j))]}";done  
}

function rand_sock_Str
{
    j=0;
    for i in {a..z};do array[$j]=$i;j=$(($j+1));done
    for i in {A..Z};do array[$j]=$i;j=$(($j+1));done
    for ((i=0;i<108;i++));do sockstrs="$sockstrs${array[$(($RANDOM%$j))]}";done  
}

function add_project()
{
    touch $project_configfile

    echo "[project]" >> $project_configfile
    echo "name=$Etmem_slide_test" >> $project_configfile
    echo "loop=1" >> $project_configfile
    echo "interval=1" >> $project_configfile
    echo "sleep=1" >> $project_configfile

    echo "sysmem_threshold=100" >> $project_configfile
    echo "swapcache_high_wmark=3" >> $project_configfile
    echo "swapcache_low_wmark=1" >> $project_configfile

    echo "" >> $project_configfile
    echo "#slide" >> $project_configfile
    echo "[engine]" >> $project_configfile
    echo "name=slide" >> $project_configfile
    echo "project=$Etmem_slide_test" >> $project_configfile

    ./bin/etmem obj add -f ${project_configfile} -s sock_slide_name

    for i in $*
    do
        rm -f $task_configfile
        touch $task_configfile
        echo "[task]" >> $task_configfile
        echo "project=$Etmem_slide_test" >> $task_configfile
        echo "engine=slide" >> $task_configfile
        echo "name=swap_test_$i" >> $task_configfile
        echo "type=name" >> $task_configfile
        echo "value = $i" >> $task_configfile
        echo "max_threads=1" >> $task_configfile
        echo "T=3" >> $task_configfile
        echo $i
        if [ $i = 'mem_swaptest' ];then
            echo "swap_threshold=1g" >> $task_configfile
            echo "swap_flag=yes" >> $task_configfile
        fi
        ./bin/etmem obj add -f ${task_configfile} -s sock_slide_name
    done
}

function start_project()
{
    ./bin/etmem project start -n ${Etmem_slide_test} -s sock_slide_name
    ./bin/etmem project show -s sock_slide_name
}

function project_name_length_larger()
{
    sed -i "s/name=test/name=project_name_length_larger_than_32/g" ${config_file_bak}
}

function project_name_length_restore()
{
    sed -i "s/name=project_name_length_larger_than_32/name=test/g" ${config_file_bak}
}

cmd_test()
{
    ./bin/etmem
    ./bin/etmem -h
    ./bin/etmem help
    ./bin/etmem project
    ./bin/etmem obj
    ./bin/etmem project -h
    ./bin/etmem obj -h
    ./bin/etmem project help
    ./bin/etmem obj help
    ./bin/etmem project other
    ./bin/etmem obj other
    ./bin/etmem obj add
    ./bin/etmem obj add -o other
    ./bin/etmem project start -o other
    ./bin/etmem project start -n project_name_length_larger_than_32
    project_name_length_larger
    ./bin/etmem obj add -f ${config_file_bak} -s sock_slide_name
    project_name_length_restore
    ./bin/etmem obj add -s sock_cmd_name
    ./bin/etmem obj add -f ${config_file}
    ./bin/etmem obj add -f ${config_file} -s sock_cmd_name
    ./bin/etmem obj add -f ${filestrs} -s sock_cmd_name
    ./bin/etmem obj add -n ${prostrs} -s sock_cmd_name
    ./bin/etmem obj add -f ${config_file} -s ${sockstrs}
    ./bin/etmem obj add -f ${config_file} -s sock_cmd_name error
    ./bin/etmem project start -n ${Etmem_slide_test} -s sock_cmd_name
    ./bin/etmem project start -n ${Etmem_slide_test}
    ./bin/etmem project start -s sock_cmd_name
    ./bin/etmem engine
    ./bin/etmem engine -h
    ./bin/etmem engine --help
    ./bin/etmem engine -s ""
    ./bin/etmem engine -s sock_name
    ./bin/etmem engine -s sock_name -n ""
    ./bin/etmem engine -s sock_name -n proj_name
    ./bin/etmem engine -s sock_name -n proj_name -e "" -t task_name
    ./bin/etmem engine -s sock_name -n proj_name -e cslide -t ""
    ./bin/etmem engine "" -s sock_name -n proj_name -e cslide -t task_name
    ./bin/etmem engine showtaskpages -s sock_name
    ./bin/etmem engine showtaskpages -s sock_name -n proj_name
    ./bin/etmem engine showtaskpages -s sock_name -n proj_name -e
    ./bin/etmem engine showtaskpages -s sock_name -n proj_name -e cslide
    ./bin/etmem engine showtaskpages -s sock_name -n proj_name -e cslide -t task_name
    ./bin/etmem engine showtaskpages -s sock_name -n proj_name -e slide -t task_name
    ./bin/etmem engine showhostpages -s sock_name -n proj_name -e cslide -t task_name
    ./bin/etmem engine showhostpages -s sock_name -n proj_name -e cslide -t ""
    ./bin/etmemd error
    ./bin/etmemd -s
    ./bin/etmemd -h
    ./bin/etmemd -l a
    ./bin/etmemd
    ./bin/etmemd -m
    ./bin/etmemd -m A
    ./bin/etmemd -h help
    ./bin/etmemd -s test_socket socket
    ./bin/etmemd -l 0 -s sock_slide_name &
    sleep 1
    ./bin/etmem engine shownothing -n proj_name -e cslide -t task_name -s sock_slide_name
    killall etmemd
}

pre_test()
{
    echo "keep the param_val unchanged"
    cp ${config_file} ${config_file_bak}
    if [ ! -d ./../build ];then
        echo -e "<build> directory \033[;31mnot exist\033[0m, pls check!"
        exit 1;
    fi
    rm -f $project_configfile
    cp ./bin/mem_swaptest ./bin/sysmem_swaptest
    ./bin/mem_swaptest &
    ./bin/sysmem_swaptest &
}

do_test()
{
    touch etmemd.log
    ./bin/etmemd -l 0 -s sock_slide_name >etmemd.log 2>&1 &
    sleep 1

    add_project $1 $2 &
    pidadd_project=$!
    wait ${pidadd_project}

    echo ""
    echo "start to test slide of etmemd"

    start_project &
    pidstart_project=$!
    wait ${pidstart_project}

    sleep 20
}

post_test()
{
    pid_swap_test=`pidof mem_swaptest`
    echo ${pid_swap_test}
    pid_sysmem_swaptest=`pidof sysmem_swaptest`
    echo ${pid_sysmem_swaptest}
    mem_vmswap=$(cat /proc/${pid_swap_test}/status |grep VmSwap |awk '{print $2}')
    echo ${mem_vmswap}

    kill -9 ${pid_swap_test}
    kill -9 ${pid_sysmem_swaptest}

    echo "now to recover env"
    rm -f ${config_file_bak}
    rm -rf etmemd.log
    killall etmemd
}

test_empty_config()
{
    ./bin/etmemd -l 0 -s dt_socket &
    etmemd_pid=$!
    sleep 1
    echo "[not_used_group]" > ../conf/conf_slide/empty_config
    echo "aaa=bbb" >> ../conf/conf_slide/empty_config
    ./bin/etmem obj add -f ../conf/conf_slide/empty_config -s dt_socket
    if [ "$?" == "0" ];then
        echo "add empty config success unexpected"
        exit 1
    fi
    ps -aux |grep -v grep |grep ${etmemd_pid}
    if [ "$?" != "0" ];then
        echo "etmemd exit unexpected after add empty config"
        exit 1
    fi
    killall etmemd
    rm -f ../conf/conf_slide/empty_config
}

test_error_config()
{
    echo "[task]" > ../conf/error_proj.config
    echo "type=xxxxx" >> ../conf/error_proj.config

    ./bin/etmemd -l 0 -s dt_socket &
    etmemd_pid=$!
    sleep 1
    ./bin/etmem obj del -f ../conf/error_proj.config -s dt_socket
    if [ "$?" == "0" ];then
        echo "add error config success unexpected"
        exit 1
    fi
    ps -aux |grep -v grep |grep ${etmemd_pid}
    if [ "$?" != "0" ];then
        echo "etmemd exit unexpected after add error config"
        exit 1
    fi
    killall etmemd
    rm -f ../conf/error_proj.config
}

test_noexist_config()
{
    ./bin/etmemd -l 0 -s dt_socket &
    etmemd_pid=$!
    sleep 1
    ./bin/etmem obj add -f ../conf/noexist.config -s dt_socket
    if [ "$?" == "0" ];then
        echo "add noexist config success unexpected"
        exit 1
    fi
    ps -aux |grep -v grep |grep ${etmemd_pid}
    if [ "$?" != "0" ];then
        echo "etmemd exit unexpected after add noexist config"
        exit 1
    fi
    killall etmemd
}

test_sig_pipe()
{
    ./bin/etmemd -l 0 -s dt_socket &
    etmemd_pid=$!
    sleep 1
    kill -13 ${etmemd_pid}
    ps -aux |grep -v grep |grep ${etmemd_pid}
    if [ "$?" != "0" ];then
        echo "etmemd exit unexpected after recv SIGPIPE"
        exit 1
    fi
    killall etmemd
}

test_etmem_help()
{
    ./bin/etmem help
    if [ "$?" != "0" ];then
        echo "etmem help return error unexpect"
        exit 1
    fi
}

rand_file_Str
rand_pro_Str
rand_sock_Str
cmd_test
pre_test
do_test mem_swaptest sysmem_swaptest
post_test

test_sig_pipe
test_empty_config
test_error_config
test_noexist_config
test_etmem_help
