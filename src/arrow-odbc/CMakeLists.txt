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

cmake_minimum_required(VERSION 3.5)
project(ArrowODBC VERSION ${WARPDRIVE_BASE_VERSION})

SET(Arrow_STATIC ON)

# Add Zlib dependencies needed by Arrow Flight. Should be pre-installed.
find_package(ZLIB REQUIRED)

# Add Protobuf dependencies needed by Arrow Flight. Should be pre-installed.
set(Protobuf_USE_STATIC_LIBS ON)
find_package(Protobuf REQUIRED)

# Add OpenSSL dependencies needed by Arrow Flight. Should be pre-installed.
# May need to set OPENSSL_ROOT_DIR first. On Mac if using brew:
# brew install openssl@1.1
# add to the cmake line -DOPENSSL_ROOT_DIR=/usr/local/Cellar/openssl@1.1/1.1.1m
if (NOT DEFINED OPENSSL_ROOT_DIR AND DEFINED APPLE)
  set(OPENSSL_ROOT_DIR /usr/local/Cellar/openssl@1.1/1.1.1m)
endif()
set(OpenSSL_USE_STATIC_LIBS ON)
set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

# Add gRPC dependencies needed by Arrow Flight. Should be pre-installed.
find_package(gRPC 1.36 CONFIG REQUIRED)


set(ARROW_ODBC_THIRDPARTY_LIBS
  spi
  arrow_odbc_spi_impl
  arrow
  arrow_flight
  arrow_flight_sql
  arrow_bundled_dependencies
  ${ZLIB_LIBRARIES}
  ${Protobuf_LIBRARIES}
  gRPC::grpc++
)

add_warpdrive_lib(arrow-odbc
              CMAKE_PACKAGE_NAME
              arrow-odbc
              PKG_CONFIG_NAME
              arrow-odbc
              SOURCES
              main.cc
              OUTPUTS
              WARPDRIVE_LIBRARIES
              PRECOMPILED_HEADERS
              DEPENDENCIES
              warpdrive_dependencies
              warpdrive
              ODBCAbstraction
              SHARED_LINK_FLAGS
              ${WARPDRIVE_SHARED_LINK_FLAGS}
              SHARED_LINK_LIBS
              ${WARPDRIVE_LINK_LIBS}
              SHARED_PRIVATE_LINK_LIBS
              ${WARPDRIVE_SHARED_PRIVATE_LINK_LIBS}
              ${ARROW_ODBC_THIRDPARTY_LIBS}
              STATIC_LINK_LIBS
              ${WARPDRIVE_STATIC_LINK_LIBS}
              ${ARROW_ODBC_THIRDPARTY_LIBS}
              SHARED_INSTALL_INTERFACE_LIBS
              ${WARPDRIVE_SHARED_INSTALL_INTERFACE_LIBS}
              STATIC_INSTALL_INTERFACE_LIBS
              ${WARPDRIVE_STATIC_INSTALL_INTERFACE_LIBS})

#target_link_libraries(FirstTest gtest gtest_main pthread)