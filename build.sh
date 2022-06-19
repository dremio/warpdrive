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
export ARROW_GIT_TAG=b050bd0d31db6412256cec3362c0d57c9732e1f2

cmake \
  -GNinja \
  -DOPENSSL_INCLUDE_DIR=/usr/include/openssl \
  -DODBCABSTRACTION_REPO=/opt/flightsql-odbc \
  -DODBCABSTRACTION_GIT_TAG=b31e46ebf4d8ff28b8a73442b516dfbf271eb0fe \
  -DARROW_GIT_REPOSITORY=$ARROW_GIT_REPOSITORY \
  -DARROW_GIT_TAG=$ARROW_GIT_TAG \
  ..
# Ninja should be called at most three times for clean builds
ninja || ninja || ninja
