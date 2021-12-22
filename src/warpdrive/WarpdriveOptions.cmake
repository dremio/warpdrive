# Options used to build warpdrive:

## Compile and link options:
### Compiler flags to append when compiling Warpdrive
set(WARPDRIVE_CXXFLAGS "")
### Build static libraries
set(WARPDRIVE_BUILD_STATIC "ON")
### Build shared libraries
set(WARPDRIVE_BUILD_SHARED "ON")
### Arbitrary string that identifies the kind of package
### (for informational purposes)
set(WARPDRIVE_PACKAGE_KIND "")
### The Warpdrive git commit id (if any)
set(WARPDRIVE_GIT_ID "93c56917c31b959e80bf24bd4db7dcb81536858c")
### The Warpdrive git commit description (if any)
set(WARPDRIVE_GIT_DESCRIPTION "")
### Exclude deprecated APIs from build
set(WARPDRIVE_NO_DEPRECATED_API "OFF")
### Use ccache when compiling (if available)
set(WARPDRIVE_USE_CCACHE "ON")
### Use ld.gold for linking on Linux (if available)
set(WARPDRIVE_USE_LD_GOLD "OFF")
### Use precompiled headers when compiling
set(WARPDRIVE_USE_PRECOMPILED_HEADERS "OFF")
### Compile-time SIMD optimization level
set(WARPDRIVE_SIMD_LEVEL "SSE4_2")
### Max runtime SIMD optimization level
set(WARPDRIVE_RUNTIME_SIMD_LEVEL "MAX")
### Arm64 arch and extensions
set(WARPDRIVE_ARMV8_ARCH "armv8-a")
### Build with Altivec if compiler has support
set(WARPDRIVE_ALTIVEC "ON")
### Build Warpdrive libraries with RATH set to $ORIGIN
set(WARPDRIVE_RPATH_ORIGIN "OFF")
### Build Warpdrive libraries with install_name set to @rpath
set(WARPDRIVE_INSTALL_NAME_RPATH "ON")
### Pass -ggdb flag to debug builds
set(WARPDRIVE_GGDB_DEBUG "ON")

## Test and benchmark options:
### Build the Warpdrive examples
set(WARPDRIVE_BUILD_EXAMPLES "OFF")
### Build the Warpdrive googletest unit tests
set(WARPDRIVE_BUILD_TESTS "OFF")
### Enable timing-sensitive tests
set(WARPDRIVE_ENABLE_TIMING_TESTS "ON")
### Build the Warpdrive integration test executables
set(WARPDRIVE_BUILD_INTEGRATION "OFF")
### Build the Warpdrive micro benchmarks
set(WARPDRIVE_BUILD_BENCHMARKS "OFF")
### Build the Warpdrive micro reference benchmarks
set(WARPDRIVE_BUILD_BENCHMARKS_REFERENCE "OFF")
### Linkage of Warpdrive libraries with unit tests executables.
set(WARPDRIVE_TEST_LINKAGE "shared")
### Build Warpdrive Fuzzing executables
set(WARPDRIVE_FUZZING "OFF")
### Enable unit tests which use large memory
set(WARPDRIVE_LARGE_MEMORY_TESTS "OFF")

## Lint options:
### Only define the lint and check-format targets
set(WARPDRIVE_ONLY_LINT "OFF")
### If off, 'quiet' flags will be passed to linting tools
set(WARPDRIVE_VERBOSE_LINT "OFF")
### Build with C++ code coverage enabled
set(WARPDRIVE_GENERATE_COVERAGE "OFF")

## Checks options:

## Project component options:
### Build the Warpdrive testing libraries
set(WARPDRIVE_TESTING "OFF")

## Thirdparty toolchain options:
### Method to use for acquiring warpdrive's build dependencies
set(WARPDRIVE_DEPENDENCY_SOURCE "AUTO")
### Show output from ExternalProjects rather than just logging to files
set(WARPDRIVE_VERBOSE_THIRDPARTY_BUILD "OFF")
### Link to shared libraries
set(WARPDRIVE_DEPENDENCY_USE_SHARED "ON")
### Rely on OpenSSL shared libraries where relevant
set(WARPDRIVE_OPENSSL_USE_SHARED "ON")

## Advanced developer options:
### Compile with extra error context (line numbers, code)
set(WARPDRIVE_EXTRA_ERROR_CONTEXT "OFF")
### If enabled install ONLY targets that have already been built. Please be
### advised that if this is enabled 'install' will fail silently on components
### that have not been built
set(WARPDRIVE_OPTIONAL_INSTALL "OFF")