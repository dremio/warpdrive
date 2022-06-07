#
# Copyright (C) 2020-2022 Dremio Corporation
#
# See “license.txt” for license information.

# Tries to find the infer module
#
# Usage of this module as follows:
#
#  find_package(InferTools)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  InferTools_PATH -
#   When set, this path is inspected instead of standard library binary locations
#   to find infer
#
# This module defines
#  INFER_BIN, The  path to the infer binary
#  INFER_FOUND, Whether infer was found

find_program(INFER_BIN
             NAMES infer
             PATHS ${InferTools_PATH}
                   $ENV{INFER_TOOLS_PATH}
                   /usr/local/bin
                   /usr/bin
                   /usr/local/homebrew/bin
                   /opt/local/bin
             NO_DEFAULT_PATH)

if("${INFER_BIN}" STREQUAL "INFER_BIN-NOTFOUND")
  set(INFER_FOUND 0)
  message(STATUS "infer not found")
else()
  set(INFER_FOUND 1)
  message(STATUS "infer found at ${INFER_BIN}")
endif()
