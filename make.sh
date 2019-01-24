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

OUTPUT_DIR="./output"
CFLAGS="-g -O2 -Wall -Werror"
#EXTRA_CFLAGS=""
OUTPUT_SUFFIX=""
makecc="cc"
if [ "$CC" ]
then
    makecc="$CC"
fi

app_list='''
test_aco_tutorial_0
test_aco_tutorial_1
test_aco_tutorial_2
test_aco_tutorial_3 -lpthread
test_aco_tutorial_4
test_aco_tutorial_5
test_aco_tutorial_6
test_aco_synopsis
test_aco_benchmark
'''

gl_opt_no_m32=""
gl_opt_no_valgrind=""

OUTPUT_DIR="$OUTPUT_DIR""//file"
OUTPUT_DIR=`dirname "$OUTPUT_DIR"`

gl_trap_str=""

function error(){
    >&2 echo "error: $*"
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

function build_f(){
    declare file
    declare cflags
    declare build_cmd
    declare tmp_ret
    declare skip_flag
    echo "OUTPUT_DIR:       $OUTPUT_DIR"
    echo "CFLAGS:           $CFLAGS"
    echo "EXTRA_CFLAGS:     $EXTRA_CFLAGS"
    echo "ACO_EXTRA_CFLAGS: $ACO_EXTRA_CFLAGS"
    echo "OUTPUT_SUFFIX:    $OUTPUT_SUFFIX"
    echo "$app_list" | grep -Po '.+$' | while read read_in
    do
        file=`echo $read_in | grep -Po "^[^\s]+"`
        cflags=`echo $read_in | sed -r 's/^\s*([^ ]+)(.*)$/\2/'`
        if [ -z "$file" ] 
        then
            continue  
        fi
        #echo "<$file>:<$cflags>:$OUTPUT_DIR:$CFLAGS:$EXTRA_CFLAGS:$OUTPUT_SUFFIX"
        build_cmd="$makecc $CFLAGS $ACO_EXTRA_CFLAGS $EXTRA_CFLAGS acosw.S aco.c $file.c $cflags -o $OUTPUT_DIR/$file$OUTPUT_SUFFIX"
        skip_flag=""
        if [ "$gl_opt_no_m32" ]
        then
            echo "$OUTPUT_SUFFIX" | grep -P "\bm32\b" &>/dev/null
            tmp_ret=$?
            if [ "$tmp_ret" -eq "0" ]
            then
                skip_flag="true"
            elif [ "$tmp_ret" -eq "1" ]
            then
                :
            else
                error "grep failed: $tmp_ret"
                exit $tmp_ret
            fi
        fi
        if [ "$gl_opt_no_valgrind" ]
        then
            echo "$OUTPUT_SUFFIX" | grep -P "\bvalgrind\b" &>/dev/null
            tmp_ret=$?
            if [ "$tmp_ret" -eq "0" ]
            then
                skip_flag="true"
            elif [ "$tmp_ret" -eq "1" ]
            then
                :
            else
                error "grep failed: $tmp_ret"
                exit $tmp_ret
            fi
        fi
        if [ "$skip_flag" ]
        then
            echo "skip    $build_cmd"
        else
            echo "        $build_cmd"
            $build_cmd
            assert "build fail"
        fi
    done
    assert "exit"
}

function usage() {
    echo "Usage: $0 [-o <no-m32|no-valgrind>] [-h]" 1>&2
    echo '''
Example:
    # default build
    bash make.sh
    # build without the i386 binary output
    bash make.sh -o no-m32
    # build without the valgrind supported binary output
    bash make.sh -o no-valgrind
    # build without the valgrind supported and i386 binary output
    bash make.sh -o no-valgrind -o no-m32
''' 1>&2
}

gl_opt_value=""
while getopts ":o:h" o; do
    case "${o}" in
        o)
            gl_opt_value=${OPTARG}
            if [ "$gl_opt_value" = "no-m32" ]
            then
                gl_opt_no_m32="true"
            elif [ "$gl_opt_value" = "no-valgrind" ]
            then
                gl_opt_no_valgrind="true"
            else
                usage
                error unknow option value of '-o'
                exit 1
            fi
            ;;
        h)
            usage
            exit 0
            ;;
        *)
            usage
            error unknow option
            exit 1
            ;;
    esac
done
shift $((OPTIND-1))

#echo "o = $gl_opt_value"
#echo "gl_opt_no_valgrind:$gl_opt_no_valgrind"
#echo "gl_opt_no_m32:$gl_opt_no_m32"

if [ -e "$OUTPUT_DIR" ]
then
    if [ -d "$OUTPUT_DIR" ]
    then
        :
    else
        error "\"$OUTPUT_DIR\" is not a directory"
        exit 1
    fi
else
    error "directory \"$OUTPUT_DIR\" doesn't exist"
    exit 1
fi

tra "echo;echo build has been interrupted"

# the matrix of the build config for later testing
# -m32 -DACO_CONFIG_SHARE_FPU_MXCSR_ENV -DACO_USE_VALGRIND
# 0 0 0
ACO_EXTRA_CFLAGS="" OUTPUT_SUFFIX="..no_valgrind.standaloneFPUenv" build_f
# 0 0 1
ACO_EXTRA_CFLAGS="-DACO_USE_VALGRIND" OUTPUT_SUFFIX="..valgrind.standaloneFPUenv" build_f
# 0 1 0
ACO_EXTRA_CFLAGS="-DACO_CONFIG_SHARE_FPU_MXCSR_ENV" OUTPUT_SUFFIX="..no_valgrind.shareFPUenv" build_f
# 0 1 1
ACO_EXTRA_CFLAGS="-DACO_CONFIG_SHARE_FPU_MXCSR_ENV -DACO_USE_VALGRIND" OUTPUT_SUFFIX="..valgrind.shareFPUenv" build_f
# 1 0 0
ACO_EXTRA_CFLAGS="-m32" OUTPUT_SUFFIX="..m32.no_valgrind.standaloneFPUenv" build_f
# 1 0 1
ACO_EXTRA_CFLAGS="-m32 -DACO_USE_VALGRIND" OUTPUT_SUFFIX="..m32.valgrind.standaloneFPUenv" build_f
# 1 1 0
ACO_EXTRA_CFLAGS="-m32 -DACO_CONFIG_SHARE_FPU_MXCSR_ENV" OUTPUT_SUFFIX="..m32.no_valgrind.shareFPUenv" build_f
# 1 1 1
ACO_EXTRA_CFLAGS="-m32 -DACO_CONFIG_SHARE_FPU_MXCSR_ENV -DACO_USE_VALGRIND" OUTPUT_SUFFIX="..m32.valgrind.shareFPUenv" build_f
