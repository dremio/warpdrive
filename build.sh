#!/usr/bin/env bash

#
# Copyright (C) 2020-2022 Dremio Corporation
#
# See “license.txt” for license information.

# This script is to facilitate building the Driver on Linux

set -e

mkdir -p _build
cd _build

ARROW_GIT_REPOSITORY="${ARROW_GIT_REPOSITORY:=https://github.com/apache/arrow}"
ARROW_GIT_TAG="${ARROW_GIT_TAG:=b050bd0d31db6412256cec3362c0d57c9732e1f2}"
ODBCABSTRACTION_REPO="${ODBCABSTRACTION_REPO:=/opt/flightsql-odbc}"
# DODBCABSTRACTION_GIT_TAG=master
# ODBCABSTRACTION_GIT_TAG="${ODBCABSTRACTION_GIT_TAG:=c0ccf3535397d22c879cafb3dea0b28a144207b8}"
ODBCABSTRACTION_GIT_TAG="${ODBCABSTRACTION_GIT_TAG:=fac4d0dd88f2fe646356673a58c207240124cbe2}"

cmake \
  -GNinja \
  -DOPENSSL_INCLUDE_DIR=/usr/include/openssl \
  -DODBCABSTRACTION_REPO=$ODBCABSTRACTION_REPO \
  -DODBCABSTRACTION_GIT_TAG=$ODBCABSTRACTION_GIT_TAG \
  -DARROW_GIT_REPOSITORY=$ARROW_GIT_REPOSITORY \
  -DARROW_GIT_TAG=$ARROW_GIT_TAG \
  ..
# Ninja should be called at most three times for clean builds
ninja || ninja || ninja
