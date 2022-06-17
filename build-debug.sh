#!/usr/bin/env bash

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


# This script is to facilitate building the Driver on Linux

set -e

mkdir -p _build
cd _build

export ARROW_GIT_REPOSITORY=/opt/arrow
export ARROW_GIT_TAG=b050bd0d31db6412256cec3362c0d57c9732e1f2

cmake \
  -GNinja \
  -DOPENSSL_INCLUDE_DIR=/usr/include/openssl \
  -DODBCABSTRACTION_REPO=/opt/flightsql-odbc \
  -DODBCABSTRACTION_GIT_TAG=c6dde1791f5e2389ecc79940d3b0339d7fccd88f \
  -DARROW_GIT_REPOSITORY=$ARROW_GIT_REPOSITORY \
  -DARROW_GIT_TAG=$ARROW_GIT_TAG \
  -Dgtest_disable_pthreads=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  ..
# Ninja should be called at most three times for clean builds
ninja || ninja || ninja
