#
# Copyright (C) 2020-2022 Dremio Corporation
#
# See “license.txt” for license information.

# Adapted from the Arrow project

add_custom_target(warpdrive-all)
add_custom_target(warpdrive)
add_custom_target(warpdrive-tests)
add_dependencies(warpdrive-all
                 warpdrive-tests)

# Enable unicode driver build
add_compile_definitions(UNICODE_SUPPORT)

include_directories(${ODBC_INCLUDE_DIRS})


# Adding unit tests part of the "warpdrive" portion of the test suite
function(ADD_WARPDRIVE_TEST REL_TEST_NAME)
  set(options)
  set(one_value_args PREFIX)
  set(multi_value_args LABELS PRECOMPILED_HEADERS)
  cmake_parse_arguments(ARG
                        "${options}"
                        "${one_value_args}"
                        "${multi_value_args}"
                        ${ARGN})

  if(ARG_PREFIX)
    set(PREFIX ${ARG_PREFIX})
  else()
    set(PREFIX "warpdrive")
  endif()

  if(ARG_LABELS)
    set(LABELS ${ARG_LABELS})
  else()
    set(LABELS "warpdrive-tests")
  endif()


  if(WARPDRIVE_BUILD_STATIC)
    set(LINK_LIBS warpdrive_static ${ODBC_LIBRARIES})
  else()
    set(LINK_LIBS warpdrive_shared ${ODBC_LIBRARIES})
  endif()
  add_fuzz_target(${REL_FUZZING_NAME}
                  PREFIX
                  ${PREFIX}
                  LINK_LIBS
                  ${LINK_LIBS}
                  ${ARG_UNPARSED_ARGUMENTS})
endfunction()

set(WARPDRIVE_SRCS
    bind.cc
    columninfo.cc
    connection.cc
    convert.cc
    descriptor.cc
#    dlg_specific.cc
#    dlg_wingui.cc
    drvconn.cc
    environ.cc
    execute.cc
    info.cc
    inouealc.cc
    loadlib.cc
    lobj.cc
    misc.cc
    multibyte.cc
    mylog.cc
    odbcapi.cc
    odbcapi30.cc
    odbcapi30w.cc
    odbcapiw.cc
    options.cc
    parse.cc
    psqlodbc.cc
    psqlsetup.cc
    qresult.cc
    results.cc
  #  setup.cc
    statement.cc
    tuple.cc
    wdapi30.cc
    wdtypes.cc
    win_unicode.cc
    xalibname.cc
)

#
# Configure the base warpdrive libraries
#

if(CXX_LINKER_SUPPORTS_VERSION_SCRIPT)
  set(WARPDRIVE_VERSION_SCRIPT_FLAGS
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/symbols.map")
  set(WARPDRIVE_SHARED_LINK_FLAGS ${WARPDRIVE_VERSION_SCRIPT_FLAGS})
endif()

set(WARPDRIVE_ALL_SRCS ${WARPDRIVE_SRCS} ${WARPDRIVE_C_SRCS})
set(WARPDRIVE_BUILD_STATIC ON)
set(WARPDRIVE_BUILD_SHARED OFF)

if(WARPDRIVE_BUILD_STATIC AND WARPDRIVE_BUNDLED_STATIC_LIBS)
  set(WARPDRIVE_BUILD_BUNDLED_DEPENDENCIES TRUE)
else()
  set(WARPDRIVE_BUILD_BUNDLED_DEPENDENCIES FALSE)
endif()

add_warpdrive_lib(warpdrive
              CMAKE_PACKAGE_NAME
              warpdrive
              PKG_CONFIG_NAME
              warpdrive
              SOURCES
              ${WARPDRIVE_ALL_SRCS}
              OUTPUTS
              WARPDRIVE_LIBRARIES
              PRECOMPILED_HEADERS
              "$<$<COMPILE_LANGUAGE:CXX>:warpdrive/pch.h>"
              DEPENDENCIES
              warpdrive_dependencies
              SHARED_LINK_FLAGS
              ${WARPDRIVE_SHARED_LINK_FLAGS}
              SHARED_LINK_LIBS
              ${WARPDRIVE_LINK_LIBS}
              SHARED_PRIVATE_LINK_LIBS
              ${WARPDRIVE_SHARED_PRIVATE_LINK_LIBS}
              STATIC_LINK_LIBS
              ${WARPDRIVE_STATIC_LINK_LIBS}
              SHARED_INSTALL_IN _LIBS
              ${WARPDRIVE_SHARED_INSTALL_INTERFACE_LIBS}
              STATIC_INSTALL_INTERFACE_LIBS
              ${WARPDRIVE_STATIC_INSTALL_INTERFACE_LIBS})

add_dependencies(warpdrive ${WARPDRIVE_LIBRARIES})
add_dependencies(warpdrive ODBCAbstraction)
include_directories(warpdrive
  ${CMAKE_CURRENT_BINARY_DIR}/../../ODBCAbstraction-prefix/src/ODBCAbstraction/odbcabstraction/include
  ${Boost_INCLUDE_DIRS}
)

foreach(LIB_TARGET ${WARPDRIVE_LIBRARIES})
  target_compile_definitions(${LIB_TARGET} PRIVATE WARPDRIVE_EXPORTING)
endforeach()

if(WARPDRIVE_BUILD_BUNDLED_DEPENDENCIES)
  warpdrive_car(_FIRST_LIB ${WARPDRIVE_BUNDLED_STATIC_LIBS})
  warpdrive_cdr(_OTHER_LIBS ${WARPDRIVE_BUNDLED_STATIC_LIBS})
  create_merged_static_lib(warpdrive_bundled_dependencies
                           NAME
                           warpdrive_bundled_dependencies
                           ROOT
                           ${_FIRST_LIB}
                           TO_MERGE
                           ${_OTHER_LIBS})
endif()

if(WARPDRIVE_TESTING)
  # that depend on gtest
  add_warpdrive_lib(warpdrive_testing
                CMAKE_PACKAGE_NAME
                WarpdriveTesting
                PKG_CONFIG_NAME
                warpdrive-testing
                SOURCES
                ${WARPDRIVE_TESTING_SRCS}
                OUTPUTS
                WARPDRIVE_TESTING_LIBRARIES
                PRECOMPILED_HEADERS
                "$<$<COMPILE_LANGUAGE:CXX>:warpdrive/pch.h>"
                DEPENDENCIES
                warpdrive_test_dependencies
                SHARED_LINK_LIBS
                warpdrive_shared
                GTest::gtest
                STATIC_LINK_LIBS
                warpdrive_static)

  add_custom_target(warpdrive_testing)
  add_dependencies(warpdrive_testing ${WARPDRIVE_TESTING_LIBRARIES})
  add_dependencies(warpdrive_testing ODBCAbstraction)

  if(WARPDRIVE_BUILD_STATIC AND WIN32)
    target_compile_definitions(warpdrive_testing_static PUBLIC WARPDRIVE_TESTING_STATIC)
  endif()

  foreach(LIB_TARGET ${WARPDRIVE_TESTING_LIBRARIES})
    target_compile_definitions(${LIB_TARGET} PRIVATE WARPDRIVE_TESTING_EXPORTING)
  endforeach()
endif()

warpdrive_install_all_headers("warpdrive")

config_summary_cmake_setters("${CMAKE_CURRENT_BINARY_DIR}/WarpdriveOptions.cmake")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WarpdriveOptions.cmake
        DESTINATION "${WARPDRIVE_CMAKE_INSTALL_DIR}")

# For backward compatibility for find_package(warpdrive)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/warpdrive-config.cmake
        DESTINATION "${WARPDRIVE_CMAKE_INSTALL_DIR}")

