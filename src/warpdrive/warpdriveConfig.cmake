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
#
# This config sets the following variables in your project::
#
#   WARPDRIVE_FULL_SO_VERSION - full shared library version of the found Warpdrive
#   WARPDRIVE_SO_VERSION - shared library version of the found Warpdrive
#   WARPDRIVE_VERSION - version of the found Warpdrive
#   WARPDRIVE_* - options used when the found Warpdrive is build such as WARPDRIVE_COMPUTE
#   Warpdrive_FOUND - true if Warpdrive found on the system
#
# This config sets the following targets in your project::
#
#   warpdrive_shared - for linked as shared library if shared library is built
#   warpdrive_static - for linked as static library if static library is built


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was warpdriveConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

set(WARPDRIVE_VERSION "0.0.1-SNAPSHOT")
set(WARPDRIVE_SO_VERSION "")
set(WARPDRIVE_FULL_SO_VERSION "")

set(WARPDRIVE_LIBRARY_PATH_SUFFIXES ";lib/;lib64;lib32;lib;bin;Library;Library/lib;Library/bin")
set(WARPDRIVE_INCLUDE_PATH_SUFFIXES "include;Library;Library/include")
set(WARPDRIVE_SYSTEM_DEPENDENCIES "")
set(WARPDRIVE_BUNDLED_STATIC_LIBS "")

include("${CMAKE_CURRENT_LIST_DIR}/WarpdriveOptions.cmake")

include(CMakeFindDependencyMacro)

# Load targets only once. If we load targets multiple times, CMake reports
# already existent target error.
if(NOT (TARGET warpdrive_shared OR TARGET warpdrive_static))
  include("${CMAKE_CURRENT_LIST_DIR}/WarpdriveTargets.cmake")

  if(TARGET warpdrive_static)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_dependency(Threads)

    if(DEFINED CMAKE_MODULE_PATH)
      set(_CMAKE_MODULE_PATH_OLD ${CMAKE_MODULE_PATH})
    endif()
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

    foreach(_DEPENDENCY ${WARPDRIVE_SYSTEM_DEPENDENCIES})
      find_dependency(${_DEPENDENCY})
    endforeach()

    if(DEFINED _CMAKE_MODULE_PATH_OLD)
      set(CMAKE_MODULE_PATH ${_CMAKE_MODULE_PATH_OLD})
      unset(_CMAKE_MODULE_PATH_OLD)
    else()
      unset(CMAKE_MODULE_PATH)
    endif()

    get_property(warpdrive_static_loc TARGET warpdrive_static PROPERTY LOCATION)
    get_filename_component(warpdrive_lib_dir ${warpdrive_static_loc} DIRECTORY)

    if(WARPDRIVE_BUNDLED_STATIC_LIBS)
      add_library(warpdrive_bundled_dependencies STATIC IMPORTED)
      set_target_properties(
        warpdrive_bundled_dependencies
        PROPERTIES
          IMPORTED_LOCATION
          "${warpdrive_lib_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}warpdrive_bundled_dependencies${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )

      get_property(warpdrive_static_interface_link_libraries
                  TARGET warpdrive_static
                  PROPERTY INTERFACE_LINK_LIBRARIES)
      set_target_properties(
        warpdrive_static PROPERTIES INTERFACE_LINK_LIBRARIES
        "${warpdrive_static_interface_link_libraries};warpdrive_bundled_dependencies")
    endif()
  endif()
endif()
