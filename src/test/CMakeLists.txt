# Licensed to the Apache Software Foundation (ASF) under one                                                                                                                       alexm@Alex-Ms-MacBook-Pro
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

project(ODBC_TESTS)
cmake_minimum_required(VERSION 3.2)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Werror=vla")

# Packages
if (APPLE)
  set(ODBC_INCLUDE_DIRS /usr/local/Cellar/libiodbc/3.52.15/include)
  set(ODBC_LIBRARIES /usr/local/Cellar/libiodbc/3.52.15/lib/libiodbc.a)
  add_compile_definitions(HAVE_LONG_LONG SQLCOLATTRIBUTE_SQLLEN WITH_IODBC)
else()
    find_package(ODBC REQUIRED)
endif()
find_package(Boost REQUIRED)
include_directories(${ODBC_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
link_libraries(${ODBC_LIBRARIES})

include(FetchContent)
FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(warpdrive_test
        src/common.cc
        src/catalogfunctions-test.cc
        src/select-test.cc
        src/result-conversions-test.cc
        src/colattribute-test.cc
        src/bindcol-test.cc
        src/diagnostic-test.cc
)
target_link_libraries(warpdrive_test gtest_main)
add_test(NAME warpdrive-tests COMMAND warpdrive_test)

