#!/bin/bash

# Copyright 2018 Sen Han <00hnes@gmail.com>
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

#ulimit -c unlimited

gl_trap_str=""

function error(){
    >&2 echo "$*"
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
    test_ct=`file * | grep -P "\bexecutable\b" | grep -Po '^[^:]+' | wc -l`
    file * | grep -P "\bexecutable\b" | grep -Po '^[^:]+' | while read infile
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

version_check_flag=`echo $1 | grep -Po "\bversion_check\b="`
version_to_check=`echo $1 | grep -Po "[0-9]+\.[0-9]+\.[0-9]+" | head -1`
version_major=`echo $version_to_check | grep -Po "^[0-9]+(?=\.)"`
version_minor=`echo $version_to_check | grep -Po "(?<=\.)[0-9]+(?=\.)"`
version_patch=`echo $version_to_check | grep -Po "(?<=\.)[0-9]+$"`
echo "$version_check_flag |$version_to_check|"
echo "|$version_major|$version_minor|$version_patch|"

makecc="cc"
if [ "$CC" ]
then
    makecc="$CC"
fi

if [ "$version_check_flag" ]
then
    if [ "$version_major" -lt 0 ] || [ "$version_minor" -lt 0 ] || [ "$version_patch" -lt 0 ]
    then
        error "synatx error: version_to_check: $version_to_check"
        exit 1
    fi
    version_check_tmpdir=`mktemp -d`
    version_check_tmpfile="$version_check_tmpdir"/tmp.c
    echo '''        #include "aco.h"
        #include <stdio.h>
        #include "aco_assert_override.h"

        int main() {''' > $version_check_tmpfile
    echo "        assert(ACO_VERSION_MAJOR == $version_major);" \
        >> $version_check_tmpfile
    echo "        assert(ACO_VERSION_MINOR == $version_minor);" \
        >> $version_check_tmpfile
    echo "        assert(ACO_VERSION_PATCH == $version_patch);" \
        >> $version_check_tmpfile
    echo "        return 0;" >> $version_check_tmpfile
    echo "        }" >> $version_check_tmpfile
    echo "$version_check_tmpfile:"
    cat $version_check_tmpfile
    $makecc -I. -g -O2 acosw.S aco.c -o "$version_check_tmpfile".bin $version_check_tmpfile
    "$version_check_tmpfile".bin
    assert "error: version_check failed: $version_to_check"
    rm -fr "$version_check_tmpdir"
    exit 0
fi

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
