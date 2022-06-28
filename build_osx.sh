#!/usr/bin/env bash

#
# Copyright (C) 2020-2022 Dremio Corporation
#
# See “license.txt” for license information.

# This script is to facilitate building the Driver on Mac OSX

set -e

vcpkg/vcpkg install --triplet x64-osx --x-install-root=vcpkg/installed

mkdir -p build
cd build

ARROW_GIT_REPOSITORY="${ARROW_GIT_REPOSITORY:=https://github.com/apache/arrow}"
ARROW_GIT_TAG="${ARROW_GIT_TAG:=b050bd0d31db6412256cec3362c0d57c9732e1f2}"
ODBCABSTRACTION_REPO="${ODBCABSTRACTION_REPO:=../flightsql-odbc}"
ODBCABSTRACTION_GIT_TAG="${ODBCABSTRACTION_GIT_TAG:=da51457dbf7612eed336db06f1fca51f0b83e12a}"

cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DVCPKG_TARGET_TRIPLET=x64-osx \
  -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DARROW_GIT_REPOSITORY=$ARROW_GIT_REPOSITORY \
  -DARROW_GIT_TAG=$ARROW_GIT_TAG \
  -DODBCABSTRACTION_REPO=$ODBCABSTRACTION_REPO \
  -DODBCABSTRACTION_GIT_TAG=$ODBCABSTRACTION_GIT_TAG \
  -GNinja \
  -DVCPKG_MANIFEST_MODE=OFF^ \
  ..

cmake --build . --config Release -j 12 || cmake --build . --config Release -j 12 || cmake --build . --config Release -j 12

cd ..
