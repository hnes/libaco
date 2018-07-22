# Copyright 2018 Sen Han 00hnes@gmail.com
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

.SILENT:
.DEFAULT_GOAL := all
.PHONY: clean conf debug dev fclean install re reconf

BUILD_TYPE ?= Release
BUILD_DIR ?= build/$(BUILD_TYPE)
BUILD_PROJ_DIR = $(BUILD_DIR)/CMakeFiles

CMAKE ?= cmake
CMAKE_FLAGS += -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
CMAKE_G_FLAGS ?= -j8

ifeq (1,$(VERBOSE))
  CMAKE_FLAGS += -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
  CMAKE_G_FLAGS += VERBOSE=1
endif

ifeq (1,$(ACO_CHECK))
  CMAKE_FLAGS += -DACO_CHECK:BOOL=ON
else
  ifeq (0,$(ACO_CHECK))
    CMAKE_FLAGS += -DACO_CHECK:BOOL=OFF
  endif
endif

ifeq (1,$(ACO_BENCH))
  CMAKE_FLAGS += -DACO_BENCH:BOOL=ON
else
  ifeq (0,$(ACO_BENCH))
    CMAKE_FLAGS += -DACO_BENCH:BOOL=OFF
  endif
endif

ifeq (1,$(ACO_SHARE_FPENV))
  CMAKE_FLAGS += -DACO_SHARE_FPENV:BOOL=ON
else
  ifeq (0,$(ACO_SHARE_FPENV))
    CMAKE_FLAGS += -DACO_SHARE_FPENV:BOOL=OFF
  endif
endif

ifeq (1,$(ACO_VALGRIND))
  CMAKE_FLAGS += -DACO_VALGRIND:BOOL=ON
else
  ifeq (0,$(ACO_VALGRIND))
    CMAKE_FLAGS += -DACO_VALGRIND:BOOL=OFF
  endif
endif

ifeq (1,$(ACO_M32))
  CMAKE_FLAGS += -DACO_M32:BOOL=ON
else
  ifeq (0,$(ACO_M32))
    CMAKE_FLAGS += -DACO_M32:BOOL=OFF
  endif
endif

ifeq (1,$(ACO_ASAN))
  CMAKE_FLAGS += -DACO_ASAN:BOOL=ON
else
  ifeq (0,$(ACO_ASAN))
    CMAKE_FLAGS += -DACO_ASAN:BOOL=OFF
  endif
endif

ifeq (1,$(ACO_COVERAGE))
  CMAKE_FLAGS += -DACO_COVERAGE:BOOL=ON
else
  ifeq (0,$(ACO_COVERAGE))
    CMAKE_FLAGS += -DACO_COVERAGE:BOOL=OFF
  endif
endif

$(BUILD_DIR) conf:
	mkdir -p $(BUILD_DIR); cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) $(CURDIR)

reconf: mrproper conf

debug dev:
	$(MAKE) BUILD_TYPE=Debug

clean:
	[ -d $(BUILD_PROJ_DIR) ] && find $(BUILD_PROJ_DIR) -name "*.o" -delete

fclean:
	[ -d $(BUILD_DIR) ] && find $(BUILD_DIR) -name "*.o" -delete

mrproper:
	$(RM) -rf $(BUILD_DIR)

re: fclean all

install:
	$(CMAKE) --build $(BUILD_DIR) --target install

%: $(BUILD_DIR)
	$(CMAKE) --build $(BUILD_DIR) --target $@ -- $(CMAKE_G_FLAGS)
