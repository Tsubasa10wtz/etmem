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
#  * Author: liuyongqiang
#  * Create: 2022-9-29
#  * Description: test shell for uswap llt
#  ******************************************************************************/

run_test_exit_value=0
usage()
{
    echo "Usage: sh test.sh [OPTIONS]"
    echo "Use test.sh to control test operation"
    echo
    echo "Misc:"
    echo "  -h, --help                      Print this help, then exit"
    echo
    echo "Compile Options:"
    echo "  -m, --cmake <option>            use cmake genenate Makefile, eg: -m(default), -mcoverage, -masan, --cmake, --cmake=coverage"
    echo "  -c, --compile                   Enable compile"
    echo "  -e, --empty                     Enable compile empty(make clean)"
    echo
    echo "TestRun Options"
    echo "  -r, --run <option>              Run all test, eg: -r, -rscreen(default), -rxml, --run, --run=screen, --run=xml"
    echo "  -s, --specify FILE              Only Run specify test executable FILE, eg: -smemory_test, --specify=memory_test"
    echo
}

ARGS=`getopt -o "hcer::m::t::s:f:" -l "help,cmake::,empty,cover-report::,run::,specify:" -n "test.sh" -- "$@"`
if [ $? != 0 ]; then
    usage
    exit 1
fi

eval set -- "${ARGS}"

if [ x"$ARGS" = x" --" ]; then
    # set default value
    COMPILE_ENABLE=no
    COVERAGE_ENABLE=no
    ASAN_ENABLE=no
    CLEAN_ENABLE=no
    RUN_TEST=yes
    RUN_MODE=screen # value: screen or xml
    COVER_REPORT_ENABLE=no
fi

while true; do
    case "${1}" in
        -h|--help)
            usage; exit 0;;
        -m|--cmake)
            CMAKE_ENABLE=yes
            shift ;;
        -c|--compile)
            COMPILE_ENABLE=yes
            shift;;
        -e|--empty)
            CLEAN_ENABLE=yes
            shift;;
        -r|--run)
            RUN_TEST=yes
            case "$2" in
                "") RUN_MODE=screen; shift 2;;
                screen) RUN_MODE=screen; shift 2;;
                xml) RUN_MODE=xml; shift 2;;
                *)echo -e "\033[;31mError\033[0m param: $2"; exit 1;;
            esac;;
        -s|--specify)
            SPECIFY=$2
            shift 2;;
        --)
            shift; break;;
    esac
done

function test_clean()
{
    echo ------------------ clean begin -----------------
    rm build -rf
    echo ------------------ clean end  ------------------
}

function test_cmake()
{
    local CMAKE_OPTION="-DCMAKE_BUILD_TYPE=Debug"
    CMAKE_OPTION="${CMAKE_OPTION} -DCONFIG_DEBUG=1"

    echo --------------- cmake begin --------------------
    if [ x"${COVERAGE_ENABLE}" = x"yes" ]; then
        CMAKE_OPTION="${CMAKE_OPTION} -DCOVERAGE_ENABLE=1"
    fi

    if [ x"${ASAN_ENABLE}" = x"yes" ]; then
        CMAKE_OPTION="${CMAKE_OPTION} -DASAN_ENABLE=1"
    fi

    if [ ! -d build ]; then
        mkdir build
    fi

    cd build
    cmake .. ${CMAKE_OPTION}
    cd -
    echo --------------- cmake end --------------------
    echo
}

function test_compile()
{
    echo --------------- compile begin --------------------
    if [ -d build ]; then
        cd build
        make
        cd -
    else
        echo -e "<build> directory \033[;31mnot exist\033[0m, pls check!"
    fi
    echo --------------- compile end   --------------------
    echo
}

function test_run_all()
{
    echo --------------- run test begin --------------------
    if [ ! -d build ]; then
        echo -e "<build> directory \033[;31mnot exist\033[0m, pls check!"
        exit 1
    fi

    cd build
    if [ x"${RUN_MODE}" = x"screen" ]; then
        RUN_MODE=0
    elif [ x"${RUN_MODE}" = x"xml" ]; then
        RUN_MODE=1
    elif [ x"${RUN_MODE}" = x"" ]; then
        RUN_MODE=0
    else
        echo -e "\033[;31mnot suport\033[0m run mode <${RUN_MODE}>"
        usage
        cd -
        exit 1
    fi

    if [ x"${SPECIFY}" = x"" ]; then
        SPECIFY=`find -name "*_llt"` # run all test
    else
        SPECIFY=`find -name "${SPECIFY}"` # run one test
    fi

    TEST_LOG=test_result.log
    >$TEST_LOG

    SLIDE_LOG=test_slide.log
    >$SLIDE_LOG

    for TEST in $SPECIFY; do
        echo $TEST
        $TEST $RUN_MODE
        if [ $? != 0 ]; then
            run_test_exit_value=1
            echo failed $TEST >> $TEST_LOG
        else
            echo passed $TEST >> $TEST_LOG
        fi
    done

    echo
    echo '#################test result begin##################'
    sed 's/passed/\x1b[32m&\x1b[0m/g; s/^failed/\x1b[31m&\x1b[0m/g' $TEST_LOG
    echo '#################test result end  ##################'
    echo
    echo --------------- run test end   --------------------
    echo
    cd -
    if [ ${run_test_exit_value} != 0 ]; then
        exit 1
    fi
}

start_seconds=$(date --date="`date +'%Y-%m-%d %H:%M:%S'`" +%s)

if [ x"${CLEAN_ENABLE}" = x"yes" ]; then
    test_clean
fi

if [ x"${CMAKE_ENABLE}" = x"yes" ]; then
    test_cmake
fi

if [ x"${COMPILE_ENABLE}" = x"yes" ]; then
    test_compile
fi

if [ x"${RUN_TEST}" = x"yes" ]; then
    test_run_all
fi

end_seconds=$(date --date="`date +'%Y-%m-%d %H:%M:%S'`" +%s)
echo -e "\033[;36muse seconds: $((end_seconds-start_seconds))\033[0m"
