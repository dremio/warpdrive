#
# Copyright (C) 2020-2022 Dremio Corporation
#
# See “license.txt” for license information.

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
        src/bindcol-test.cc
        src/catalogfunctions-test.cc
        src/colattribute-test.cc
        src/common.cc
        src/connect-test.cc
        src/diagnostic-test.cc
        src/result-conversions-test.cc
        src/select-test.cc
        src/statement-functions-test.cc
        src/wchar-char-test-utf8.cc
)

target_link_libraries(warpdrive_test gtest_main)
add_test(NAME warpdrive-tests COMMAND warpdrive_test)

