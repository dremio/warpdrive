#!/usr/bin/env bash

#
# Copyright (C) 2020-2022 Dremio Corporation
#
# See “license.txt” for license information.

# This script is to facilitate building the Driver on Linux

set -e

mkdir -p _build
cd _build

export ARROW_GIT_REPOSITORY=/opt/arrow
export ARROW_GIT_TAG=master

cmake \
  -GNinja \
  -DOPENSSL_INCLUDE_DIR=/usr/include/openssl \
  -DODBCABSTRACTION_REPO=/opt/flightsql-odbc \
  -DODBCABSTRACTION_GIT_TAG=master \
  ..
# Ninja should be called at most three times for clean builds
ninja || ninja || ninja
