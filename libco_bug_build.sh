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

set -o errexit

rm -fr coctx_swap.S coctx_swap.S.origin
wget https://raw.githubusercontent.com/Tencent/libco/master/coctx_swap.S
ls -hl coctx_swap.S
cp coctx_swap.S coctx_swap.S.origin
patch -p0 coctx_swap.S  coctx_swap.patch
set +o errexit
diff coctx_swap.S coctx_swap.S.origin
set -o errexit

echo ""
echo "------------------"
echo "require gcc >= 5.0"
echo "------------------"
echo ""
gcc -Wall -Werror -D ACO_CONFIG_SHARE_FPU_MXCSR_ENV \
    -g -O2 coctx_swap.S aco.c test_aco_benchmark.c -o test_libco_benchmark
gcc -Wall -Werror -m32 -D ACO_CONFIG_SHARE_FPU_MXCSR_ENV \
    -g -O2 coctx_swap.S aco.c test_aco_benchmark.c -o test_m32_libco_benchmark
gcc -Wall -Werror -D ACO_CONFIG_SHARE_FPU_MXCSR_ENV \
    -g -O2 coctx_swap.S aco.c test_libco_bug_0.c -o test_libco_bug_0
gcc -Wall -Werror -m32 -D ACO_CONFIG_SHARE_FPU_MXCSR_ENV \
    -g -O2 coctx_swap.S aco.c test_libco_bug_0.c -o test_m32_libco_bug_0
