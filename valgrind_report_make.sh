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

#echo ""
#echo "------------------"
#echo "require gcc >= 5.0"
#echo "------------------"
#echo ""

gcc -D ACO_USE_VALGRIND -g -O0 acosw.S aco.c \
    val_standalone_stack_co.c -o val_standalone_stack_co
gcc -D ACO_USE_VALGRIND -g -O0 acosw.S aco.c \
    val_copystack_co.c -o val_copystack_co

time valgrind --leak-check=full --error-exitcode=2 --tool=memcheck ./val_standalone_stack_co

time valgrind --leak-check=full --error-exitcode=2 --tool=memcheck ./val_copystack_co
