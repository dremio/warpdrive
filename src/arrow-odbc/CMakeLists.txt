#
# Copyright (C) 2020-2022 Dremio Corporation
#
# See “license.txt” for license information.

cmake_minimum_required(VERSION 3.5)
project(ArrowODBC VERSION ${WARPDRIVE_BASE_VERSION})

SET(Arrow_STATIC ON)

# Add Zlib dependencies needed by Arrow Flight. Should be pre-installed.
find_package(ZLIB REQUIRED)

# Add Protobuf dependencies needed by Arrow Flight. Should be pre-installed.
# To install on Linux (tested on Linux Mint 20.2):  $ apt install libprotobuf-dev
if (NOT DEFINED MSVC)
  set(Protobuf_USE_STATIC_LIBS ON)
endif()
find_package(Protobuf REQUIRED)

# Add OpenSSL dependencies needed by Arrow Flight. Should be pre-installed.
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

# Add gRPC dependencies needed by Arrow Flight. Should be pre-installed.
find_package(gRPC 1.36 CONFIG REQUIRED)

# Define the version number information.
if(DEFINED ENV{WARPDRIVE_BUILD_VERSION})
  set(WARPDRIVE_BUILD_VERSION "$ENV{WARPDRIVE_BUILD_VERSION}")
else()
  set(WARPDRIVE_BUILD_VERSION "${WARPDRIVE_VERSION_MAJOR}.${WARPDRIVE_VERSION_MINOR}.${WARPDRIVE_VERSION_PATCH}.0")
endif()
add_compile_definitions(WARPDRIVE_BUILD_VERSION="${WARPDRIVE_BUILD_VERSION}")

if (MSVC)
  # the following definitions stop arrow from using __declspec when staticly linking and will break on windows without them
  add_compile_definitions(ARROW_STATIC ARROW_FLIGHT_STATIC)
endif()

if (MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "/MD")
  set(CMAKE_CXX_FLAGS_DEBUG "/MDd")
  set(ARROW_LIBS
    arrow_flight_sql_static
    arrow_flight_static
    arrow_static
  )
else()
  set(ARROW_LIBS
    arrow_flight_sql
    arrow_flight
    arrow
    arrow_bundled_dependencies
  )
endif()

find_package(Boost REQUIRED COMPONENTS locale)

set(ARROW_ODBC_THIRDPARTY_LIBS
  arrow_odbc_spi_impl
  odbcabstraction
  ${ARROW_LIBS}
  gRPC::grpc++
  ${ZLIB_LIBRARIES}
  ${Protobuf_LIBRARIES}
  ${ODBC_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)

if (MSVC)
  find_package(Boost REQUIRED COMPONENTS locale)
  list(APPEND ARROW_ODBC_THIRDPARTY_LIBS
    ${Boost_LIBRARIES}
    ${CMAKE_CURRENT_BINARY_DIR}/../../${CMAKE_BUILD_TYPE}/$(Configuration)/warpdrive_static.lib
    legacy_stdio_definitions)
endif()

set(ARROW_ODBC_SOURCES
  main.cc
)

if (MSVC)
  list(APPEND ARROW_ODBC_SOURCES arrow-odbc.def)

  # Configure Resource Compiler
  enable_language(RC)
  # use English language (0x409) in resource compiler
  set(rc_flags "/l0x409")
  # fix rc.exe invocations because of usage of add_definitions()
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> ${rc_flags} <DEFINES> /fo<OBJECT> <SOURCE>")

  # Add version info resources
  # More info at: https://docs.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource
  set(WARPDRIVE_RC_PRODUCTVERSION "${WARPDRIVE_BUILD_VERSION}")
  string(REPLACE "." "," WARPDRIVE_RC_FILEVERSION "${WARPDRIVE_RC_PRODUCTVERSION}")
  configure_file(version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
  list(APPEND ARROW_ODBC_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

# Add symbols file
if(APPLE)
 # set(WARPDRIVE_SHARED_LINK_FLAGS "-exported_symbols_list ${CMAKE_CURRENT_BINARY_DIR}/arrow-odbc.exports ${WARPDRIVE_SHARED_LINK_FLAGS}")
endif()

add_warpdrive_lib(arrow-odbc
              CMAKE_PACKAGE_NAME
              arrow-odbc
              PKG_CONFIG_NAME
              arrow-odbc
              SOURCES
              ${ARROW_ODBC_SOURCES}
              OUTPUTS
              WARPDRIVE_LIBRARIES
              #warpdrive
              PRECOMPILED_HEADERS
              BUILD_STATIC
              OFF
              BUILD_SHARED
              ON
              DEPENDENCIES
              warpdrive_dependencies
              warpdrive
              ODBCAbstraction
              SHARED_LINK_FLAGS
              ${WARPDRIVE_SHARED_LINK_FLAGS}
              SHARED_LINK_LIBS
              ${WARPDRIVE_LINK_LIBS}
#              warpdrive
              SHARED_PRIVATE_LINK_LIBS
              ${WARPDRIVE_SHARED_PRIVATE_LINK_LIBS}
              ${ARROW_ODBC_THIRDPARTY_LIBS}
              STATIC_LINK_LIBS
              ${WARPDRIVE_STATIC_LINK_LIBS}
              ${ARROW_ODBC_THIRDPARTY_LIBS}
              SHARED_INSTALL_INTERFACE_LIBS
              ${WARPDRIVE_SHARED_INSTALL_INTERFACE_LIBS}
              STATIC_INSTALL_INTERFACE_LIBS
              ${WARPDRIVE_STATIC_INSTALL_INTERFACE_LIBS}
              FORCE_LOAD_LIB
              warpdrive)

include_directories(arrow-odbc 
  ${CMAKE_CURRENT_BINARY_DIR}/../warpdrive
  ${CMAKE_CURRENT_BINARY_DIR}/../../ODBCAbstraction-prefix/src/ODBCAbstraction/flight_sql/include
  ${CMAKE_CURRENT_BINARY_DIR}/../../ODBCAbstraction-prefix/src/ODBCAbstraction/odbcabstraction/include
  ${ODBC_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

link_directories(${BUILD_OUTPUT_ROOT_DIRECTORY})

#target_link_libraries(FirstTest gtest gtest_main pthread)
