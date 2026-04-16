# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

# Memory leak detection and debugging flags for tests
set(CMAKE_CXX_FLAGS "-O0 -fsanitize=undefined,address -Wall -g -ggdb3")
set(CMAKE_C_FLAGS "-O0 -fsanitize=undefined,address -Wall -g -ggdb3")
set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=undefined,address")
set(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=undefined,address")

add_subdirectory(panels)
