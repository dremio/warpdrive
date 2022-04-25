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

# Adapted from Apache Arrow.

cmake_minimum_required(VERSION 3.5)
message(STATUS "Building using CMake version: ${CMAKE_VERSION}")

# Compiler id for Apple Clang is now AppleClang.
# https://www.cmake.org/cmake/help/latest/policy/CMP0025.html
cmake_policy(SET CMP0025 NEW)

# Only interpret if() arguments as variables or keywords when unquoted.
# https://www.cmake.org/cmake/help/latest/policy/CMP0054.html
cmake_policy(SET CMP0054 NEW)

# Support new if() IN_LIST operator.
# https://www.cmake.org/cmake/help/latest/policy/CMP0057.html
cmake_policy(SET CMP0057 NEW)

# Adapted from Apache Kudu: https://github.com/apache/kudu/commit/bd549e13743a51013585
# Honor visibility properties for all target types.
# https://www.cmake.org/cmake/help/latest/policy/CMP0063.html
cmake_policy(SET CMP0063 NEW)

# RPATH settings on macOS do not affect install_name.
# https://cmake.org/cmake/help/latest/policy/CMP0068.html
if(POLICY CMP0068)
  cmake_policy(SET CMP0068 NEW)
endif()

# find_package() uses <PackageName>_ROOT variables.
# https://cmake.org/cmake/help/latest/policy/CMP0074.html
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

set (CMAKE_CXX_STANDARD 11)

set(WARPDRIVE_VERSION "0.0.1-SNAPSHOT")

string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" WARPDRIVE_BASE_VERSION "${WARPDRIVE_VERSION}")

# if no build build type is specified, default to release builds
if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build.")
endif()
string(TOLOWER ${CMAKE_BUILD_TYPE} LOWERCASE_BUILD_TYPE)
string(TOUPPER ${CMAKE_BUILD_TYPE} UPPERCASE_BUILD_TYPE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")

# this must be included before the project() command, because of the way
# vcpkg (ab)uses CMAKE_TOOLCHAIN_FILE to inject its logic into CMake
if(WARPDRIVE_DEPENDENCY_SOURCE STREQUAL "VCPKG")
  include(Usevcpkg)
endif()

project(warpdrive VERSION "${WARPDRIVE_BASE_VERSION}")

set(WARPDRIVE_VERSION_MAJOR "${warpdrive_VERSION_MAJOR}")
set(WARPDRIVE_VERSION_MINOR "${warpdrive_VERSION_MINOR}")
set(WARPDRIVE_VERSION_PATCH "${warpdrive_VERSION_PATCH}")
if(WAPRDRIVE_VERSION_MAJOR STREQUAL ""
   OR WARPDRIVE_VERSION_MINOR STREQUAL ""
   OR WARPDRIVE_VERSION_PATCH STREQUAL "")
  message(FATAL_ERROR "Failed to determine Warpdrive version from '${WARPDRIVE_VERSION}'")
endif()

message(STATUS "Warpdrive version: "
               "${WARPDRIVE_VERSION_MAJOR}.${WARPDRIVE_VERSION_MINOR}.${WARPDRIVE_VERSION_PATCH} "
               "(full: '${WARPDRIVE_VERSION}')")

set(WARPDRIVE_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(WARPDRIVE_BINARY_DIR ${PROJECT_BINARY_DIR})

include(CMakePackageConfigHelpers)
include(CMakeParseArguments)
include(ExternalProject)
include(FindPackageHandleStandardArgs)

include(GNUInstallDirs)

set(BUILD_SUPPORT_DIR "${CMAKE_SOURCE_DIR}/build-support")

set(WARPDRIVE_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(WARPDRIVE_DOC_DIR "share/doc/${PROJECT_NAME}")

set(WARPDRIVE_LLVM_VERSIONS
    "13.0"
    "12.0"
    "11.1"
    "11.0"
    "10"
    "9"
    "8"
    "7")
list(GET WARPDRIVE_LLVM_VERSIONS 0 WARPDRIVE_LLVM_VERSION_PRIMARY)
string(REGEX REPLACE "^([0-9]+)(\\..+)?" "\\1" WARPDRIVE_LLVM_VERSION_PRIMARY_MAJOR
                     "${WARPDRIVE_LLVM_VERSION_PRIMARY}")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/.env WARPDRIVE_ENV)
string(REGEX MATCH "CLANG_TOOLS=[^\n]+" WARPDRIVE_ENV_CLANG_TOOLS_VERSION "${WARPDRIVE_ENV}")
string(REGEX REPLACE "^CLANG_TOOLS=" "" WARPDRIVE_CLANG_TOOLS_VERSION
                     "${WARPDRIVE_ENV_CLANG_TOOLS_VERSION}")
string(REGEX REPLACE "^([0-9]+)(\\..+)?" "\\1" WARPDRIVE_CLANG_TOOLS_VERSION_MAJOR
                     "${WARPDRIVE_CLANG_TOOLS_VERSION}")

if(APPLE)
  find_program(BREW_BIN brew)
  if(BREW_BIN)
    execute_process(COMMAND ${BREW_BIN} --prefix
                            "llvm@${WARPDRIVE_LLVM_VERSION_PRIMARY_MAJOR}"
                    OUTPUT_VARIABLE LLVM_BREW_PREFIX
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT LLVM_BREW_PREFIX)
      execute_process(COMMAND ${BREW_BIN} --prefix llvm
                      OUTPUT_VARIABLE LLVM_BREW_PREFIX
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()

    execute_process(COMMAND ${BREW_BIN} --prefix "llvm@${WARPDRIVE_CLANG_TOOLS_VERSION_MAJOR}"
                    OUTPUT_VARIABLE CLANG_TOOLS_BREW_PREFIX
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT CLANG_TOOLS_BREW_PREFIX)
      execute_process(COMMAND ${BREW_BIN} --prefix llvm
                      OUTPUT_VARIABLE CLANG_TOOLS_BREW_PREFIX
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
  endif()
endif()

if(WIN32 AND NOT MINGW)
  # This is used to handle builds using e.g. clang in an MSVC setting.
  set(MSVC_TOOLCHAIN TRUE)
else()
  set(MSVC_TOOLCHAIN FALSE)
endif()

find_package(ClangTools)
find_package(InferTools)
if("$ENV{CMAKE_EXPORT_COMPILE_COMMANDS}" STREQUAL "1"
   OR CLANG_TIDY_FOUND
   OR INFER_FOUND)
  # Generate a Clang compile_commands.json "compilation database" file for use
  # with various development tools, such as Vim's YouCompleteMe plugin.
  # See http://clang.llvm.org/docs/JSONCompilationDatabase.html
  set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
endif()

# ----------------------------------------------------------------------
# cmake options
include(DefineOptions)


if(WARPDRIVE_USE_PRECOMPILED_HEADERS AND ${CMAKE_VERSION} VERSION_LESS "3.16.0")
  message(WARNING "Precompiled headers need CMake 3.16.0 or later, disabling")
  set(WARPDRIVE_USE_PRECOMPILED_HEADERS OFF)
endif()

if(WARPDRIVE_OPTIONAL_INSTALL)
  # Don't make the "install" target depend on the "all" target
  set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

  set(INSTALL_IS_OPTIONAL OPTIONAL)
endif()

#
# "make lint" target
#
if(NOT WARPDRIVE_VERBOSE_LINT)
  set(WARPDRIVE_LINT_QUIET "--quiet")
endif()

if(NOT LINT_EXCLUSIONS_FILE)
  # source files matching a glob from a line in this file
  # will be excluded from linting (cpplint, clang-tidy, clang-format)
  set(LINT_EXCLUSIONS_FILE ${BUILD_SUPPORT_DIR}/lint_exclusions.txt)
endif()

find_program(CPPLINT_BIN
             NAMES cpplint cpplint.py
             HINTS ${BUILD_SUPPORT_DIR})
message(STATUS "Found cpplint executable at ${CPPLINT_BIN}")

set(COMMON_LINT_OPTIONS
    --exclude_globs
    ${LINT_EXCLUSIONS_FILE}
    --source_dir
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    --source_dir
    ${CMAKE_CURRENT_SOURCE_DIR}/examples
    --source_dir
    ${CMAKE_CURRENT_SOURCE_DIR}/tools)

add_custom_target(lint
                  ${PYTHON_EXECUTABLE}
                  ${BUILD_SUPPORT_DIR}/run_cpplint.py
                  --cpplint_binary
                  ${CPPLINT_BIN}
                  ${COMMON_LINT_OPTIONS}
                  ${WARPDRIVE_LINT_QUIET})

#
# "make format" and "make check-format" targets
#
if(${CLANG_FORMAT_FOUND})
  # runs clang format and updates files in place.
  add_custom_target(format
                    ${PYTHON_EXECUTABLE}
                    ${BUILD_SUPPORT_DIR}/run_clang_format.py
                    --clang_format_binary
                    ${CLANG_FORMAT_BIN}
                    ${COMMON_LINT_OPTIONS}
                    --fix
                    ${WARPDRIVE_LINT_QUIET})

  # runs clang format and exits with a non-zero exit code if any files need to be reformatted
  add_custom_target(check-format
                    ${PYTHON_EXECUTABLE}
                    ${BUILD_SUPPORT_DIR}/run_clang_format.py
                    --clang_format_binary
                    ${CLANG_FORMAT_BIN}
                    ${COMMON_LINT_OPTIONS}
                    ${WARPDRIVE_LINT_QUIET})
endif()

add_custom_target(lint_cpp_cli ${PYTHON_EXECUTABLE} ${BUILD_SUPPORT_DIR}/lint_cpp_cli.py
                               ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(WARPDRIVE_LINT_ONLY)
  message("WARPDRIVE_LINT_ONLY was specified, this is only a partial build directory")
  return()
endif()

#
# "make clang-tidy" and "make check-clang-tidy" targets
#
if(${CLANG_TIDY_FOUND})
  # TODO check to make sure .clang-tidy is being respected

  # runs clang-tidy and attempts to fix any warning automatically
  add_custom_target(clang-tidy
                    ${PYTHON_EXECUTABLE}
                    ${BUILD_SUPPORT_DIR}/run_clang_tidy.py
                    --clang_tidy_binary
                    ${CLANG_TIDY_BIN}
                    --compile_commands
                    ${CMAKE_BINARY_DIR}/compile_commands.json
                    ${COMMON_LINT_OPTIONS}
                    --fix
                    ${WARPDRIVE_LINT_QUIET})

  # runs clang-tidy and exits with a non-zero exit code if any errors are found.
  add_custom_target(check-clang-tidy
                    ${PYTHON_EXECUTABLE}
                    ${BUILD_SUPPORT_DIR}/run_clang_tidy.py
                    --clang_tidy_binary
                    ${CLANG_TIDY_BIN}
                    --compile_commands
                    ${CMAKE_BINARY_DIR}/compile_commands.json
                    ${COMMON_LINT_OPTIONS}
                    ${WARPDRIVE_LINT_QUIET})
endif()

if(UNIX)
  add_custom_target(iwyu ${BUILD_SUPPORT_DIR}/iwyu/iwyu.sh)
  add_custom_target(iwyu-all ${BUILD_SUPPORT_DIR}/iwyu/iwyu.sh all)
endif(UNIX)

#
# Set up various options
#


if(NOT WARPDRIVE_BUILD_TESTS)
  set(NO_TESTS 1)
else()
  add_custom_target(all-tests)
  add_custom_target(unittest
                    ctest
                    -j4
                    -L
                    unittest
                    --output-on-failure)
  add_dependencies(unittest all-tests)
endif()

if(WARPDRIVE_ENABLE_TIMING_TESTS)
  add_definitions(-DWARPDRIVE_WITH_TIMING_TESTS)
endif()

if(NOT WARPDRIVE_BUILD_EXAMPLES)
  set(NO_EXAMPLES 1)
endif()

if(NOT WARPDRIVE_FUZZING)
  set(NO_FUZZING 1)
endif()

if(WARPDRIVE_LARGE_MEMORY_TESTS)
  add_definitions(-DWARPDRIVE_LARGE_MEMORY_TESTS)
endif()

if(WARPDRIVE_TEST_MEMCHECK)
  add_definitions(-DWARPDRIVE_VALGRIND)
endif()

if(WARPDRIVE_USE_UBSAN)
  add_definitions(-DWARPDRIVE_UBSAN)
endif()

#
# Compiler flags
#

if(WARPDRIVE_NO_DEPRECATED_API)
  add_definitions(-DWARPDRIVE_NO_DEPRECATED_API)
endif()

if(WARPDRIVE_EXTRA_ERROR_CONTEXT)
  add_definitions(-DWARPDRIVE_EXTRA_ERROR_CONTEXT)
endif()

include(SetupCxxFlags)

#
# Linker flags
#

# Localize thirdparty symbols using a linker version script. This hides them
# from the client application. The OS X linker does not support the
# version-script option.
if(CMAKE_VERSION VERSION_LESS 3.18)
  if(APPLE OR WIN32)
    set(CXX_LINKER_SUPPORTS_VERSION_SCRIPT FALSE)
  else()
    set(CXX_LINKER_SUPPORTS_VERSION_SCRIPT TRUE)
  endif()
else()
  include(CheckLinkerFlag)
  check_linker_flag(CXX
                    "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/warpdrive/symbols.map"
                    CXX_LINKER_SUPPORTS_VERSION_SCRIPT)
endif()

#
# Build output directory
#

# set compile output directory
string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_SUBDIR_NAME)

# If build in-source, create the latest symlink. If build out-of-source, which is
# preferred, simply output the binaries in the build folder
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  set(BUILD_OUTPUT_ROOT_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}/build/${BUILD_SUBDIR_NAME}/")
  # Link build/latest to the current build directory, to avoid developers
  # accidentally running the latest debug build when in fact they're building
  # release builds.
  file(MAKE_DIRECTORY ${BUILD_OUTPUT_ROOT_DIRECTORY})
  if(NOT APPLE)
    set(MORE_ARGS "-T")
  endif()
  execute_process(COMMAND ln ${MORE_ARGS} -sf ${BUILD_OUTPUT_ROOT_DIRECTORY}
                          ${CMAKE_CURRENT_BINARY_DIR}/build/latest)
else()
  set(BUILD_OUTPUT_ROOT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${BUILD_SUBDIR_NAME}/")
endif()

# where to put generated archives (.a files)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BUILD_OUTPUT_ROOT_DIRECTORY}")
set(ARCHIVE_OUTPUT_DIRECTORY "${BUILD_OUTPUT_ROOT_DIRECTORY}")

# where to put generated libraries (.so files)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILD_OUTPUT_ROOT_DIRECTORY}")
set(LIBRARY_OUTPUT_DIRECTORY "${BUILD_OUTPUT_ROOT_DIRECTORY}")

# where to put generated binaries
set(EXECUTABLE_OUTPUT_PATH "${BUILD_OUTPUT_ROOT_DIRECTORY}")

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Xcode projects support multi-configuration builds.  This forces a single output directory
  # when building with Xcode that is consistent with single-configuration Makefile driven build.
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${UPPERCASE_BUILD_TYPE}
      "${BUILD_OUTPUT_ROOT_DIRECTORY}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${UPPERCASE_BUILD_TYPE}
      "${BUILD_OUTPUT_ROOT_DIRECTORY}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${UPPERCASE_BUILD_TYPE}
      "${BUILD_OUTPUT_ROOT_DIRECTORY}")
endif()

#
# Dependencies
#

include(BuildUtils)
enable_testing()

# For warpdrive.pc. Requires.private and Libs.private are used when
# "pkg-config --libs --static warpdrive" is used.
set(WARPDRIVE_PC_REQUIRES_PRIVATE)
set(WARPDRIVE_PC_LIBS_PRIVATE)


# Add common flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARPDRIVE_CXXFLAGS}")

# For any C code, use the same flags. These flags don't contain
# C++ specific flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CXX_COMMON_FLAGS} ${WARPDRIVE_CXXFLAGS}")

# Remove --std=c++11 to avoid errors from C compilers
string(REPLACE "-std=c++11" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})

# Add C++-only flags, like -std=c++11
set(CMAKE_CXX_FLAGS "${CXX_ONLY_FLAGS} ${CMAKE_CXX_FLAGS}")

# Code coverage
if("${WARPDRIVE_GENERATE_COVERAGE}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -DCOVERAGE_BUILD")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -DCOVERAGE_BUILD")
endif()

# CMAKE_CXX_FLAGS now fully assembled
message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(src)

#
# "make ctags" target
#
if(UNIX)
  add_custom_target(ctags ctags -R --languages=c++,c)
endif(UNIX)

#
# "make etags" target
#
if(UNIX)
  add_custom_target(tags
                    etags
                    --members
                    --declarations
                    `find
                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                    -name
                    \\*.cc
                    -or
                    -name
                    \\*.hh
                    -or
                    -name
                    \\*.cpp
                    -or
                    -name
                    \\*.h
                    -or
                    -name
                    \\*.c
                    -or
                    -name
                    \\*.f`)
  add_custom_target(etags DEPENDS tags)
endif(UNIX)

#
# "make cscope" target
#
if(UNIX)
  add_custom_target(cscope
                    find
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    (-name
                     \\*.cc
                     -or
                     -name
                     \\*.hh
                     -or
                     -name
                     \\*.cpp
                     -or
                     -name
                     \\*.h
                     -or
                     -name
                     \\*.c
                     -or
                     -name
                     \\*.f)
                    -exec
                    echo
                    \"{}\"
                    \;
                    >
                    cscope.files
                    &&
                    cscope
                    -q
                    -b
                    VERBATIM)
endif(UNIX)

#
# "make infer" target
#

if(${INFER_FOUND})
  # runs infer capture
  add_custom_target(infer ${BUILD_SUPPORT_DIR}/run-infer.sh ${INFER_BIN}
                          ${CMAKE_BINARY_DIR}/compile_commands.json 1)
  # runs infer analyze
  add_custom_target(infer-analyze ${BUILD_SUPPORT_DIR}/run-infer.sh ${INFER_BIN}
                                  ${CMAKE_BINARY_DIR}/compile_commands.json 2)
  # runs infer report
  add_custom_target(infer-report ${BUILD_SUPPORT_DIR}/run-infer.sh ${INFER_BIN}
                                 ${CMAKE_BINARY_DIR}/compile_commands.json 3)
endif()

# Add ODBC dependencies.
# ODBC_LIBRARIES is really more for applications to link to the DM. We overwrite it to
# just pull in the ODBC installer library for SQLGetPrivateProfileString.
if(APPLE)
  set(ODBC_INCLUDE_DIRS /usr/local/Cellar/libiodbc/3.52.15/include)
  set(ODBC_LIBRARIES /usr/local/Cellar/libiodbc/3.52.15/lib/libiodbcinst.dylib)
  add_compile_definitions(HAVE_LONG_LONG SQLCOLATTRIBUTE_SQLLEN WITH_IODBC)
else()
  find_package(ODBC REQUIRED)
  if(UNIX)
    add_compile_definitions(WITH_UNIXODBC)
    set(ODBC_LIBRARIES odbcinst)
  elseif(MSVC)
  set(ODBC_LIBRARIES odbccp32)    
  endif()
endif()

# Add OpenSSL dependencies needed by ODBCAbstraction. Should be pre-installed.
# May need to set OPENSSL_ROOT_DIR first. On Mac if using brew:
# brew install openssl@1.1
# add to the cmake line -DOPENSSL_ROOT_DIR=/usr/local/Cellar/openssl@1.1/1.1.1m
if (NOT DEFINED OPENSSL_ROOT_DIR AND DEFINED APPLE)
  set(OPENSSL_ROOT_DIR /usr/local/Cellar/openssl@1.1/1.1.1m)
endif()
if (NOT DEFINED MSVC)
  set(OpenSSL_USE_STATIC_LIBS ON)
  set(OPENSSL_USE_STATIC_LIBS ON)
endif()
find_package(OpenSSL REQUIRED)

define_option(ODBCABSTRACTION_REPO "Flight SQL ODBC repo" "")
if (NOT ODBCABSTRACTION_REPO)
  # Add dependency on the ODBC Abstraction, which also gets and builds Arrow.
  define_option(GITHUB_ACCESS_TOKEN "Github access token" "")
  if (GITHUB_ACCESS_TOKEN)
    set(ODBCABSTRACTION_REPO https://${GITHUB_ACCESS_TOKEN}@github.com/jcralmeida/flightsql-odbc.git)
  else()
    set(ODBCABSTRACTION_REPO https://github.com/jcralmeida/flightsql-odbc.git)
  endif()
endif()

define_option(ODBCABSTRACTION_GIT_TAG, "Tag for the ODBC Abstraction project" "master")

define_option(ARROW_GIT_REPOSITORY "Arrow repo" "")
if (NOT ARROW_GIT_REPOSITORY)
  set(ARROW_GIT_REPOSITORY "$ENV{ARROW_GIT_REPOSITORY}")
endif()
define_option(ARROW_GIT_TAG "Tag for the Arrow repository" "")
if (NOT ARROW_GIT_TAG)
  set(ARROW_GIT_TAG "$ENV{ARROW_GIT_TAG}")
endif()

if (MSVC)
  set(ODBCABSTRACTION_CMAKE_ARGS
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DARROW_GIT_REPOSITORY=${ARROW_GIT_REPOSITORY}
    -DARROW_GIT_TAG=${ARROW_GIT_TAG}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}
    -DVCPKG_MANIFEST_MODE=${VCPKG_MANIFEST_MODE}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  )

  ExternalProject_Add(ODBCAbstraction
    GIT_REPOSITORY ${ODBCABSTRACTION_REPO}
    GIT_TAG ${ODBCABSTRACTION_GIT_TAG}
    CMAKE_ARGS ${ODBCABSTRACTION_CMAKE_ARGS}
    CMAKE_GENERATOR "Visual Studio 17 2022"
    CMAKE_GENERATOR_PLATFORM ${CMAKE_VS_PLATFORM_NAME}
    INSTALL_COMMAND cmake -E echo "Skipping install step.")
    #TODO: Fix the INSTALL operation in the ODBCAbstraction project.
else()
  set(ODBCABSTRACTION_CMAKE_ARGS "\
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DARROW_GIT_REPOSITORY=${ARROW_GIT_REPOSITORY} \
    -DARROW_GIT_TAG=${ARROW_GIT_TAG} \
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
  )

  ExternalProject_Add(ODBCAbstraction
    GIT_REPOSITORY ${ODBCABSTRACTION_REPO}
    GIT_TAG ${ODBCABSTRACTION_GIT_TAG}
    CMAKE_ARGS ${ODBCABSTRACTION_CMAKE_ARGS}
    INSTALL_COMMAND cmake -E echo "Skipping install step.")
    #TODO: Fix the INSTALL operation in the ODBCAbstraction project.
endif()

find_package(Threads REQUIRED)

# Add Boost dependencies. Should be pre-installed (Brew on Mac).
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Add Arrow library build path and ODBCAbstraction library path.
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(ARROW_LIB_DIR lib64)
else()
  set(ARROW_LIB_DIR lib)
endif()
link_directories(${CMAKE_CURRENT_BINARY_DIR}/ODBCAbstraction-prefix/src/ODBCAbstraction-build/flight_sql/ApacheArrow-prefix/src/ApacheArrow-install/${ARROW_LIB_DIR})
link_directories(${CMAKE_CURRENT_BINARY_DIR}/ODBCAbstraction-prefix/src/ODBCAbstraction-build/lib)
if (MSVC)
  link_directories(${CMAKE_CURRENT_BINARY_DIR}/ODBCAbstraction-prefix/src/ODBCAbstraction-build)
  link_directories(${CMAKE_CURRENT_BINARY_DIR}/ODBCAbstraction-prefix/src/ODBCAbstraction-build/$(Configuration)/lib)
endif()
link_directories(${BUILD_OUTPUT_ROOT_DIRECTORY})
list(APPEND WARPDRIVE_LINK_LIBS odbcabstraction)
list(APPEND WARPDRIVE_STATIC_LINK_LIBS odbcabstraction)
list(APPEND WARPDRIVE_STATIC_INSTALL_INTERFACE_LIBS odbcabstraction)
list(APPEND WARPDRIVE_SHARED_LINK_LIBS odbcabstraction Threads::Threads)
list(APPEND WARPDRIVE_SHARED_INSTALL_INTERFACE_LIBS odbcabstraction)

# Add include directories for _only_ the ODBCAbstraction SPI.
include_directories(${CMAKE_CURRENT_BINARY_DIR}/ODBCAbstraction-prefix/src/ODBCAbstraction/odbcabstraction/include)

#
# Linker and Dependencies
#

# Libraries to link statically with libwarpdrive.so
set(WARPDRIVE_LINK_LIBS)
set(WARPDRIVE_STATIC_LINK_LIBS)
set(WARPDRIVE_STATIC_INSTALL_INTERFACE_LIBS)

if(WARPDRIVE_USE_OPENSSL)
  set(WARPDRIVE_OPENSSL_LIBS OpenSSL::Crypto OpenSSL::SSL)
  list(APPEND WARPDRIVE_LINK_LIBS ${WARPDRIVE_OPENSSL_LIBS})
  list(APPEND WARPDRIVE_STATIC_LINK_LIBS ${WARPDRIVE_OPENSSL_LIBS})
  list(APPEND WARPDRIVE_STATIC_INSTALL_INTERFACE_LIBS ${WARPDRIVE_OPENSSL_LIBS})
endif()


add_custom_target(warpdrive_dependencies)
add_custom_target(warpdrive_benchmark_dependencies)
add_custom_target(warpdrive_test_dependencies)

# WARPDRIVE-4581: CMake can be finicky about invoking the ExternalProject builds
# for some of the library dependencies, so we "nuke it from orbit" by making
# the toolchain dependency explicit using these "dependencies" targets
#add_dependencies(warpdrive_dependencies toolchain)
#add_dependencies(warpdrive_test_dependencies toolchain-tests)

if(WARPDRIVE_STATIC_LINK_LIBS)
  add_dependencies(warpdrive_dependencies ${WARPDRIVE_STATIC_LINK_LIBS})
endif()

set(WARPDRIVE_SHARED_PRIVATE_LINK_LIBS ${WARPDRIVE_STATIC_LINK_LIBS})

if(NOT MSVC_TOOLCHAIN)
  list(APPEND WARPDRIVE_LINK_LIBS ${CMAKE_DL_LIBS})
  list(APPEND WARPDRIVE_SHARED_INSTALL_INTERFACE_LIBS ${CMAKE_DL_LIBS})
endif()

set(WARPDRIVE_TEST_LINK_TOOLCHAIN
    GTest::gtest_main
    GTest::gtest
    GTest::gmock
    ${BOOST_FILESYSTEM_LIBRARY}
    ${BOOST_SYSTEM_LIBRARY})

if(WARPDRIVE_BUILD_TESTS)
  add_dependencies(warpdrive_test_dependencies ${WARPDRVE_TEST_LINK_TOOLCHAIN})
endif()

if(WARPDRIVE_BUILD_BENCHMARKS)
  # Some benchmarks use gtest
  add_dependencies(warpdrive_benchmark_dependencies warpdrive_test_dependencies
                   toolchain-benchmarks)
endif()

set(WARPDRIVE_TEST_STATIC_LINK_LIBS warpdrive_testing_static warpdrive_static ${WARPDRIVE_LINK_LIBS}
                                ${WARPDRIVE_TEST_LINK_TOOLCHAIN})

set(WARPDRIVE_TEST_SHARED_LINK_LIBS warpdrive_testing_shared warpdrive_shared ${WARPDRIVE_LINK_LIBS}
                                ${WARPDRIVE_TEST_LINK_TOOLCHAIN})

if(NOT MSVC)
  set(WARPDRIVE_TEST_SHARED_LINK_LIBS ${WARPDRIVE_TEST_SHARED_LINK_LIBS} ${CMAKE_DL_LIBS})
endif()

if("${WARPDRIVE_TEST_LINKAGE}" STREQUAL "shared")
  if(WARPDRIVE_BUILD_TESTS AND NOT WARPDRIVE_BUILD_SHARED)
    message(FATAL_ERROR "If using shared linkage for unit tests, must also \
pass WARPDRIVE_BUILD_SHARED=on")
  endif()
  # Use shared linking for unit tests if it's available
  set(WARPDRIVE_TEST_LINK_LIBS ${WARPDRIVE_TEST_SHARED_LINK_LIBS})
  set(WARPDRIVE_EXAMPLE_LINK_LIBS warpdrive_shared)
else()
  if(WARPDRIVE_BUILD_TESTS AND NOT WARPDRIVE_BUILD_STATIC)
    message(FATAL_ERROR "If using static linkage for unit tests, must also \
pass WARPDRIVE_BUILD_STATIC=on")
  endif()
  set(WARPDRIVE_TEST_LINK_LIBS ${WARPDRIVE_TEST_STATIC_LINK_LIBS})
  set(WARPDRIVE_EXAMPLE_LINK_LIBS warpdrive_static)
endif()


# ----------------------------------------------------------------------
# Handle platform-related libraries like -pthread

set(WARPDRIVE_SYSTEM_LINK_LIBS)

#list(APPEND WARPDRIVE_SYSTEM_LINK_LIBS Threads::Threads)
##if(CMAKE_THREAD_LIBS_INIT)
 # string(APPEND WARPDRIVE_PC_LIBS_PRIVATE " ${CMAKE_THREAD_LIBS_INIT}")
#endif()

if(WIN32)
  # Winsock
  list(APPEND WARPDRIVE_SYSTEM_LINK_LIBS "ws2_32.dll")
endif()

if(NOT WIN32 AND NOT APPLE)
  # Pass -lrt on Linux only
  list(APPEND WARPDRIVE_SYSTEM_LINK_LIBS rt)
endif()

list(APPEND WARPDRIVE_LINK_LIBS ${WARPDRIVE_SYSTEM_LINK_LIBS})
list(APPEND WARPDRIVE_STATIC_LINK_LIBS ${WARPDRIVE_SYSTEM_LINK_LIBS})
list(APPEND WARPDRIVE_STATIC_INSTALL_INTERFACE_LIBS ${WARPDRIVE_SYSTEM_LINK_LIBS})

#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE.txt
 #             ${CMAKE_CURRENT_SOURCE_DIR}/../NOTICE.txt
 #             ${CMAKE_CURRENT_SOURCE_DIR}/README.md DESTINATION "${WARPDRIVE_DOC_DIR}")

add_subdirectory(src/warpdrive)
add_subdirectory(src/arrow-odbc)
add_subdirectory(src/test)

#
# Validate and print out Warpdrive configuration options
#

validate_config()
config_summary_message()
if(${WARPDRIVE_BUILD_CONFIG_SUMMARY_JSON})
  config_summary_json()
endif()
