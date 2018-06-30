#!/bin/bash

# Copyright 2018 Sen Han 00hnes@gmail.com
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ulimit -c unlimited

gl_trap_str=""

function error(){
    echo "$*" > /proc/self/fd/2
}

function assert(){
    if [ "0" -ne "$?" ]
    then
        error "$0:""$*"
        exit 1
    fi
}

function tra(){
    gl_trap_str="$gl_trap_str""$1"
    trap "$gl_trap_str exit 1;" INT
    assert "$LINENO:trap failed:$gl_trap_str:$1"
}

function untra(){
    trap - INT
    assert "$LINENO:untrap failed:$gl_trap_str:$1"
}

function test_f_is_exclude_app(){
    declare infile
    declare main_name
    infile=$1
    main_name=`echo "$infile" | sed -r "s|(.*)\.\.(.*)|\1|"`
    if [ -z "$infile" -o -z "$main_name" ]
    then
        error "$0:""$*"
        exit 1
    fi
    if [ "$main_name" = "test_aco_benchmark" ]
    then
        return 0
    else
        return 1
    fi  
}

function test_f_handle_exit_code(){
    declare infile
    declare errc
    declare main_name
    declare intended_to_abort
    infile=$1
    errc=$2
    if [ -z "$infile" ]
    then
        error test_f_handle_exit_code illegal input       
        exit 1
    fi
    if [ -z "$errc" -o "$errc" -lt "0" ]
    then
        error test_f_handle_exit_code illegal input
        exit 1
    fi
    main_name=`echo "$infile" | sed -r "s|(.*)\.\.(.*)|\1|"`
    intended_to_abort=""
    if [ "$main_name" = "test_aco_tutorial_4" -o "$main_name" = "test_aco_tutorial_5" ]
    then
        intended_to_abort="true"
    fi
    if [ "$intended_to_abort" -a "$errc" -ne "134" ]
    then
        echo ""
        echo test $infile intended to abort failed:$errc
        exit $errc
    fi
    if [ -z "$intended_to_abort" -a "$errc" -ne "0" ]
    then
        echo ""
        echo test $infile failed:$errc
        exit $errc
    fi
    if [ "$intended_to_abort" ] 
    then
        echo test $infile intended to abort success:$errc
    else
        echo test $infile success
    fi
}

function test_f(){
    declare valgrind_support
    declare errc
    declare test_ct
    declare infile
    test_ct=`file * | grep -P "ELF.*executable" | grep -Po '^[^:]+' | wc -l`
    file * | grep -P "ELF.*executable" | grep -Po '^[^:]+' | while read infile
    do
        test_f_is_exclude_app "$infile"
        if [ "0" -eq "$?" ]
        then
            echo "----" $infile is in the exclude app list, bypass its test
            echo
            continue
        fi
        valgrind_support=`echo "$infile" | grep -Po '.*\.\.(.*)' | sed -r "s|(.*)\.\.(.*)|\2|" | grep -Po '\bvalgrind\b'`
        if [ -z "$valgrind_support" ]
        then
            echo "----" $infile start":"
            time ./$infile
            errc="$?"
            test_f_handle_exit_code $infile $errc
        else
            echo "----" $infile memcheck start":"
            time valgrind --leak-check=full --error-exitcode=2 --tool=memcheck ./$infile
            errc="$?"
            test_f_handle_exit_code $infile $errc
        fi
        echo
    done
    errc="$?"
    if [ "$errc" -ne "0" ]
    then
        exit "$errc"
    fi
    if [ "$test_ct" -ne "0" ]
    then
        echo all the "$test_ct" tests had passed, OK and cheers!
    else
        echo no test need to do in current directory: "`pwd`"
    fi
}

tra "echo;echo test had been interrupted;exit 0;"

# test loop
while true
do
    echo "---- time:"`date`
    test_f
    errc="$?"
    if [ "$errc" -ne 0 ]
    then
        exit $errc
    fi   
    if [ "$1" != "loop" ]
    then
        exit 0
    fi
    echo ""
    echo "----" start all tests again
    sleep 1
done
