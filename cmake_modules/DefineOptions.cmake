#
# Copyright (C) 2020-2022 Dremio Corporation
#
# See “license.txt” for license information.

macro(set_option_category name)
  set(WARPDRIVE_OPTION_CATEGORY ${name})
  list(APPEND "WARPDRIVE_OPTION_CATEGORIES" ${name})
endmacro()

function(check_description_length name description)
  foreach(description_line ${description})
    string(LENGTH ${description_line} line_length)
    if(${line_length} GREATER 80)
      message(FATAL_ERROR "description for ${name} contained a\n\
        line ${line_length} characters long!\n\
        (max is 80). Split it into more lines with semicolons")
    endif()
  endforeach()
endfunction()

function(list_join lst glue out)
  if("${${lst}}" STREQUAL "")
    set(${out}
        ""
        PARENT_SCOPE)
    return()
  endif()

  list(GET ${lst} 0 joined)
  list(REMOVE_AT ${lst} 0)
  foreach(item ${${lst}})
    set(joined "${joined}${glue}${item}")
  endforeach()
  set(${out}
      ${joined}
      PARENT_SCOPE)
endfunction()

macro(define_option name description default)
  check_description_length(${name} ${description})
  list_join(description "\n" multiline_description)

  option(${name} "${multiline_description}" ${default})

  list(APPEND "WARPDRIVE_${WARPDRIVE_OPTION_CATEGORY}_OPTION_NAMES" ${name})
  set("${name}_OPTION_DESCRIPTION" ${description})
  set("${name}_OPTION_DEFAULT" ${default})
  set("${name}_OPTION_TYPE" "bool")
endmacro()

macro(define_option_string name description default)
  check_description_length(${name} ${description})
  list_join(description "\n" multiline_description)

  set(${name}
      ${default}
      CACHE STRING "${multiline_description}")

  list(APPEND "WARPDRIVE_${WARPDRIVE_OPTION_CATEGORY}_OPTION_NAMES" ${name})
  set("${name}_OPTION_DESCRIPTION" ${description})
  set("${name}_OPTION_DEFAULT" "\"${default}\"")
  set("${name}_OPTION_TYPE" "string")
  set("${name}_OPTION_POSSIBLE_VALUES" ${ARGN})

  list_join("${name}_OPTION_POSSIBLE_VALUES" "|" "${name}_OPTION_ENUM")
  if(NOT ("${${name}_OPTION_ENUM}" STREQUAL ""))
    set_property(CACHE ${name} PROPERTY STRINGS "${name}_OPTION_POSSIBLE_VALUES")
  endif()
endmacro()

# Top level cmake dir
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  #----------------------------------------------------------------------
  set_option_category("Compile and link")

  define_option_string(WARPDRIVE_CXXFLAGS "Compiler flags to append when compiling Warpdrive" "")

  define_option(WARPDRIVE_BUILD_STATIC "Build static libraries" ON)

  define_option(WARPDRIVE_BUILD_SHARED "Build shared libraries" ON)

  define_option_string(WARPDRIVE_PACKAGE_KIND
                       "Arbitrary string that identifies the kind of package;\
(for informational purposes)" "")

  define_option_string(WARPDRIVE_GIT_ID "The Warpdrive git commit id (if any)" "")

  define_option_string(WARPDRIVE_GIT_DESCRIPTION "The Warpdrive git commit description (if any)"
                       "")

  define_option(WARPDRIVE_NO_DEPRECATED_API "Exclude deprecated APIs from build" OFF)

  define_option(WARPDRIVE_USE_CCACHE "Use ccache when compiling (if available)" ON)

  define_option(WARPDRIVE_USE_LD_GOLD "Use ld.gold for linking on Linux (if available)" OFF)

  define_option(WARPDRIVE_USE_PRECOMPILED_HEADERS "Use precompiled headers when compiling"
                OFF)

  define_option_string(WARPDRIVE_SIMD_LEVEL
                       "Compile-time SIMD optimization level"
                       "DEFAULT" # default to SSE4_2 on x86, NEON on Arm, NONE otherwise
                       "NONE"
                       "SSE4_2"
                       "AVX2"
                       "AVX512"
                       "NEON"
                       "DEFAULT")

  define_option_string(WARPDRIVE_RUNTIME_SIMD_LEVEL
                       "Max runtime SIMD optimization level"
                       "MAX" # default to max supported by compiler
                       "NONE"
                       "SSE4_2"
                       "AVX2"
                       "AVX512"
                       "MAX")

  # Arm64 architectures and extensions can lead to exploding combinations.
  # So set it directly through cmake command line.
  #
  # If you change this, you need to change the definition in
  # python/CMakeLists.txt too.
  define_option_string(WARPDRIVE_ARMV8_ARCH
                       "Arm64 arch and extensions"
                       "armv8-a" # Default
                       "armv8-a"
                       "armv8-a+crc+crypto")

  define_option(WARPDRIVE_ALTIVEC "Build with Altivec if compiler has support" ON)

  define_option(WARPDRIVE_RPATH_ORIGIN "Build Warpdrive libraries with RATH set to \$ORIGIN" OFF)

  define_option(WARPDRIVE_INSTALL_NAME_RPATH
                "Build Warpdrive libraries with install_name set to @rpath" ON)

  define_option(WARPDRIVE_GGDB_DEBUG "Pass -ggdb flag to debug builds" ON)

  #----------------------------------------------------------------------
  set_option_category("Test and benchmark")

  define_option(WARPDRIVE_BUILD_EXAMPLES "Build the Warpdrive examples" OFF)

  define_option(WARPDRIVE_BUILD_TESTS "Build the Warpdrive googletest unit tests" OFF)

  define_option(WARPDRIVE_ENABLE_TIMING_TESTS "Enable timing-sensitive tests" ON)

  define_option(WARPDRIVE_BUILD_INTEGRATION "Build the Warpdrive integration test executables"
                OFF)

  define_option(WARPDRIVE_BUILD_BENCHMARKS "Build the Warpdrive micro benchmarks" OFF)

  # Reference benchmarks are used to compare to naive implementation, or
  # discover various hardware limits.
  define_option(WARPDRIVE_BUILD_BENCHMARKS_REFERENCE
                "Build the Warpdrive micro reference benchmarks" OFF)

  if(WARPDRIVE_BUILD_SHARED)
    set(WARPDRIVE_TEST_LINKAGE_DEFAULT "shared")
  else()
    set(WARPDRIVE_TEST_LINKAGE_DEFAULT "static")
  endif()

  define_option_string(WARPDRIVE_TEST_LINKAGE
                       "Linkage of Warpdrive libraries with unit tests executables."
                       "${WARPDRIVE_TEST_LINKAGE_DEFAULT}"
                       "shared"
                       "static")

  define_option(WARPDRIVE_FUZZING "Build Warpdrive Fuzzing executables" OFF)

  define_option(WARPDRIVE_LARGE_MEMORY_TESTS "Enable unit tests which use large memory" OFF)

  #----------------------------------------------------------------------
  set_option_category("Lint")

  define_option(WARPDRIVE_ONLY_LINT "Only define the lint and check-format targets" OFF)

  define_option(WARPDRIVE_VERBOSE_LINT
                "If off, 'quiet' flags will be passed to linting tools" OFF)

  define_option(WARPDRIVE_GENERATE_COVERAGE "Build with C++ code coverage enabled" OFF)

  #----------------------------------------------------------------------
  set_option_category("Checks")


  #----------------------------------------------------------------------
  set_option_category("Project component")

  define_option(WARPDRIVE_TESTING "Build the Warpdrive testing libraries" OFF)

  #----------------------------------------------------------------------
  set_option_category("Thirdparty toolchain")

  # Determine how we will look for dependencies
  # * AUTO: Guess which packaging systems we're running in and pull the
  #   dependencies from there. Build any missing ones through the
  #   ExternalProject setup. This is the default unless the CONDA_PREFIX
  #   environment variable is set, in which case the CONDA method is used
  # * BUNDLED: Build dependencies through CMake's ExternalProject facility. If
  #   you wish to build individual dependencies from source instead of using
  #   one of the other methods, pass -D$NAME_SOURCE=BUNDLED
  # * SYSTEM: Use CMake's find_package and find_library without any custom
  #   paths. If individual packages are on non-default locations, you can pass
  #   $NAME_ROOT arguments to CMake, or set environment variables for the same
  #   with CMake 3.11 and higher.  If your system packages are in a non-default
  #   location, or if you are using a non-standard toolchain, you can also pass
  #   WARPDRIVE_PACKAGE_PREFIX to set the *_ROOT variables to look in that
  #   directory
  # * CONDA: Same as SYSTEM but set all *_ROOT variables to
  #   ENV{CONDA_PREFIX}. If this is run within an active conda environment,
  #   then ENV{CONDA_PREFIX} will be used for dependencies unless
  #   WARPDRIVE_DEPENDENCY_SOURCE is set explicitly to one of the other options
  # * VCPKG: Searches for dependencies installed by vcpkg.
  # * BREW: Use SYSTEM but search for select packages with brew.
  if(NOT "$ENV{CONDA_PREFIX}" STREQUAL "")
    set(WARPDRIVE_DEPENDENCY_SOURCE_DEFAULT "CONDA")
  else()
    set(WARPDRIVE_DEPENDENCY_SOURCE_DEFAULT "AUTO")
  endif()
  define_option_string(WARPDRIVE_DEPENDENCY_SOURCE
                       "Method to use for acquiring warpdrive's build dependencies"
                       "${WARPDRIVE_DEPENDENCY_SOURCE_DEFAULT}"
                       "AUTO"
                       "BUNDLED"
                       "SYSTEM"
                       "CONDA"
                       "VCPKG"
                       "BREW")

  define_option(WARPDRIVE_VERBOSE_THIRDPARTY_BUILD
                "Show output from ExternalProjects rather than just logging to files" OFF)

  define_option(WARPDRIVE_DEPENDENCY_USE_SHARED "Link to shared libraries" ON)

  define_option(WARPDRIVE_OPENSSL_USE_SHARED
                "Rely on OpenSSL shared libraries where relevant"
                ${WARPDRIVE_DEPENDENCY_USE_SHARED})

  #----------------------------------------------------------------------
  if(MSVC_TOOLCHAIN)
    set_option_category("MSVC")

    define_option(MSVC_LINK_VERBOSE
                  "Pass verbose linking options when linking libraries and executables"
                  OFF)
    set(SNAPPY_MSVC_STATIC_LIB_SUFFIX_DEFAULT "")
    define_option_string(SNAPPY_MSVC_STATIC_LIB_SUFFIX
                         "Snappy static lib suffix used on Windows with MSVC"
                         "${SNAPPY_MSVC_STATIC_LIB_SUFFIX_DEFAULT}")

    define_option_string(LZ4_MSVC_STATIC_LIB_SUFFIX
                         "Lz4 static lib suffix used on Windows with MSVC" "_static")

    define_option_string(ZSTD_MSVC_STATIC_LIB_SUFFIX
                         "ZStd static lib suffix used on Windows with MSVC" "_static")

    define_option(WARPDRIVE_USE_STATIC_CRT "Build Warpdrive with statically linked CRT" OFF)
  endif()

  #----------------------------------------------------------------------
  set_option_category("Advanced developer")

  define_option(WARPDRIVE_EXTRA_ERROR_CONTEXT
                "Compile with extra error context (line numbers, code)" OFF)

  define_option(WARPDRIVE_OPTIONAL_INSTALL
                "If enabled install ONLY targets that have already been built. Please be;\
advised that if this is enabled 'install' will fail silently on components;\
that have not been built"
                OFF)

  option(WARPDRIVE_BUILD_CONFIG_SUMMARY_JSON "Summarize build configuration in a JSON file"
         ON)
endif()

macro(validate_config)
  foreach(category ${WARPDRIVE_OPTION_CATEGORIES})
    set(option_names ${WARPDRIVE_${category}_OPTION_NAMES})

    foreach(name ${option_names})
      set(possible_values ${${name}_OPTION_POSSIBLE_VALUES})
      set(value "${${name}}")
      if(possible_values)
        if(NOT "${value}" IN_LIST possible_values)
          message(FATAL_ERROR "Configuration option ${name} got invalid value '${value}'. "
                              "Allowed values: ${${name}_OPTION_ENUM}.")
        endif()
      endif()
    endforeach()

  endforeach()
endmacro()

macro(config_summary_message)
  message(STATUS "---------------------------------------------------------------------")
  message(STATUS "Warpdrive version:                                 ${WARPDRIVE_VERSION}")
  message(STATUS)
  message(STATUS "Build configuration summary:")

  message(STATUS "  Generator: ${CMAKE_GENERATOR}")
  message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
  message(STATUS "  Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
  message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
  if(${CMAKE_EXPORT_COMPILE_COMMANDS})
    message(STATUS "  Compile commands: ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json"
    )
  endif()

  foreach(category ${WARPDRIVE_OPTION_CATEGORIES})

    message(STATUS)
    message(STATUS "${category} options:")
    message(STATUS)

    set(option_names ${WARPDRIVE_${category}_OPTION_NAMES})

    foreach(name ${option_names})
      set(value "${${name}}")
      if("${value}" STREQUAL "")
        set(value "\"\"")
      endif()

      set(description ${${name}_OPTION_DESCRIPTION})

      if(NOT ("${${name}_OPTION_ENUM}" STREQUAL ""))
        set(summary "=${value} [default=${${name}_OPTION_ENUM}]")
      else()
        set(summary "=${value} [default=${${name}_OPTION_DEFAULT}]")
      endif()

      message(STATUS "  ${name}${summary}")
      foreach(description_line ${description})
        message(STATUS "      ${description_line}")
      endforeach()
    endforeach()

  endforeach()

endmacro()

macro(config_summary_json)
  set(summary "${CMAKE_CURRENT_BINARY_DIR}/cmake_summary.json")
  message(STATUS "  Outputting build configuration summary to ${summary}")
  file(WRITE ${summary} "{\n")

  foreach(category ${WARPDRIVE_OPTION_CATEGORIES})
    foreach(name ${WARPDRIVE_${category}_OPTION_NAMES})
      file(APPEND ${summary} "\"${name}\": \"${${name}}\",\n")
    endforeach()
  endforeach()

  file(APPEND ${summary} "\"generator\": \"${CMAKE_GENERATOR}\",\n")
  file(APPEND ${summary} "\"build_type\": \"${CMAKE_BUILD_TYPE}\",\n")
  file(APPEND ${summary} "\"source_dir\": \"${CMAKE_CURRENT_SOURCE_DIR}\",\n")
  if(${CMAKE_EXPORT_COMPILE_COMMANDS})
    file(APPEND ${summary} "\"compile_commands\": "
                           "\"${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json\",\n")
  endif()
  file(APPEND ${summary} "\"install_prefix\": \"${CMAKE_INSTALL_PREFIX}\",\n")
  file(APPEND ${summary} "\"warpdrive_version\": \"${WARPDRIVE_VERSION}\"\n")
  file(APPEND ${summary} "}\n")
endmacro()

macro(config_summary_cmake_setters path)
  file(WRITE ${path} "# Options used to build warpdrive:")

  foreach(category ${WARPDRIVE_OPTION_CATEGORIES})
    file(APPEND ${path} "\n\n## ${category} options:")
    foreach(name ${WARPDRIVE_${category}_OPTION_NAMES})
      set(description ${${name}_OPTION_DESCRIPTION})
      foreach(description_line ${description})
        file(APPEND ${path} "\n### ${description_line}")
      endforeach()
      file(APPEND ${path} "\nset(${name} \"${${name}}\")")
    endforeach()
  endforeach()

endmacro()

#----------------------------------------------------------------------
# Compute default values for omitted variables

if(NOT WARPDRIVE_GIT_ID)
  execute_process(COMMAND "git" "log" "-n1" "--format=%H"
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  OUTPUT_VARIABLE WARPDRIVE_GIT_ID
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(NOT WARPDRIVE_GIT_DESCRIPTION)
  execute_process(COMMAND "git" "describe" "--tags" "--dirty"
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                  ERROR_QUIET
                  OUTPUT_VARIABLE WARPDRIVE_GIT_DESCRIPTION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
