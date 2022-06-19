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
export ARROW_GIT_TAG=6bd50bc8e1a938eeaacd44f00d886dd14a8c4a5b

cmake \
  -GNinja \
  -DOPENSSL_INCLUDE_DIR=/usr/include/openssl \
  -DODBCABSTRACTION_REPO=/opt/flightsql-odbc \
  -DODBCABSTRACTION_GIT_TAG=b31e46ebf4d8ff28b8a73442b516dfbf271eb0fe \
  -DARROW_GIT_REPOSITORY=$ARROW_GIT_REPOSITORY \
  -DARROW_GIT_TAG=$ARROW_GIT_TAG \
  -Dgtest_disable_pthreads=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  ..
# Ninja should be called at most three times for clean builds
ninja || ninja || ninja
