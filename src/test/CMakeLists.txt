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
        src/statement-functions-test.cc
)
target_link_libraries(warpdrive_test gtest_main)
add_test(NAME warpdrive-tests COMMAND warpdrive_test)

# Test executables
# add_executable(alter src/alter-test.cc src/common.cc)
# add_executable(arraybinding src/arraybinding-test.cc src/common.cc)
# add_executable(bindcol src/bindcol-test.cc src/common.cc)
# add_executable(bookmark src/bookmark-test.cc src/common.cc)
# add_executable(boolsaschar src/boolsaschar-test.cc src/common.cc)
# add_executable(catalogfunctions src/catalogfunctions-test.cc src/common.cc)
# add_executable(colattribute src/colattribute-test.cc src/common.cc)
# add_executable(commands src/commands-test.cc src/common.cc)
# add_executable(connect src/connect-test.cc src/common.cc)
# add_executable(cte src/cte-test.cc src/common.cc)
# add_executable(cursor-block-delete src/cursor-block-delete-test.cc src/common.cc)
# add_executable(cursor-commit src/cursor-commit-test.cc src/common.cc)
# add_executable(cursor-movement src/cursor-movement-test.cc src/common.cc)
# add_executable(cursor-name src/cursor-name-test.cc src/common.cc)
# add_executable(cursors src/cursors-test.cc src/common.cc)
# add_executable(cvtnulldate src/cvtnulldate-test.cc src/common.cc)
# add_executable(dataatexecution src/dataatexecution-test.cc src/common.cc)
# add_executable(declare-fetch-block src/declare-fetch-block-test.cc src/common.cc)
# add_executable(declare-fetch-commit src/declare-fetch-commit-test.cc src/common.cc)
# add_executable(deprecated src/deprecated-test.cc src/common.cc)
# add_executable(diagnostic src/diagnostic-test.cc src/common.cc)
# add_executable(error-rollback src/error-rollback-test.cc src/common.cc)
# add_executable(errors src/errors-test.cc src/common.cc)
# add_executable(fetch-refcursors src/fetch-refcursors-test.cc src/common.cc)
# add_executable(getresult src/getresult-test.cc src/common.cc)
# add_executable(identity src/identity-test.cc src/common.cc)
# add_executable(insertreturning src/insertreturning-test.cc src/common.cc)
# add_executable(large-object-data-at-exec src/large-object-data-at-exec-test.cc src/common.cc)
# add_executable(large-object src/large-object-test.cc src/common.cc)
# add_executable(lfconversion src/lfconversion-test.cc src/common.cc)
# add_executable(multistmt src/multistmt-test.cc src/common.cc)
# add_executable(notice src/notice-test.cc src/common.cc)
# add_executable(numeric src/numeric-test.cc src/common.cc)
# add_executable(odbc-escapes src/odbc-escapes-test.cc src/common.cc)
# add_executable(param-conversions src/param-conversions-test.cc src/common.cc)
# # add_executable(params-batch-exec src/params-batch-exec-test.cc src/common.cc)
# add_executable(params src/params-test.cc src/common.cc)
# add_executable(parse src/parse-test.cc src/common.cc)
# add_executable(positioned-update src/positioned-update-test.cc src/common.cc)
# add_executable(premature src/premature-test.cc src/common.cc)
# add_executable(prepare src/prepare-test.cc src/common.cc)
# add_executable(quotes src/quotes-test.cc src/common.cc)
# add_executable(select src/select-test.cc src/common.cc)
# add_executable(stmthandles src/stmthandles-test.cc src/common.cc)
# add_executable(update src/update-test.cc src/common.cc)
# # add_executable(wchar-char-eucjp src/wchar-char-test-eucjp.c src/common.cc)
# # add_executable(wchar-char-sjis src/wchar-char-test-sjis.c src/common.cc)
# # add_executable(wchar-char-utf8 src/wchar-char-test-utf8.c src/common.cc)
# # add_executable(wchar-char src/wchar-char-test.cc src/common.cc)
