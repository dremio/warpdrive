
@rem
@rem Copyright 2015 the original author or authors.
@rem
@rem Licensed under the Apache License, Version 2.0 (the "License");
@rem you may not use this file except in compliance with the License.
@rem You may obtain a copy of the License at
@rem
@rem      https://www.apache.org/licenses/LICENSE-2.0
@rem
@rem Unless required by applicable law or agreed to in writing, software
@rem distributed under the License is distributed on an "AS IS" BASIS,
@rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
@rem See the License for the specific language governing permissions and
@rem limitations under the License.
@rem

@REM Please define ARROW_GIT_REPOSITORY to be the arrow repository. If this is a local repo,
@REM use forward slashes instead of backslashes.

@REM Please define VCPKG_ROOT to be the directory with a built vcpkg. This path should use
@REM forward slashes instead of backslashes.

@ECHO OFF

%VCPKG_ROOT%\vcpkg.exe install --triplet x86-windows --x-install-root=%VCPKG_ROOT%/installed

if exist ".\build" del build /q

mkdir build

cd build

if NOT DEFINED ARROW_GIT_REPOSITORY SET ARROW_GIT_REPOSITORY = "https://github.com/apache/arrow"
if NOT DEFINED ARROW_GIT_TAG SET AROW_GIT_TAG = "b050bd0d31db6412256cec3362c0d57c9732e1f2"
if NOT DEFINED ODBCABSTRACTION_REPO SET ODBCABSTRACTION_REPO = "../flightsql-odbc"
if NOT DEFINED ODBCABSTRACTION_GIT_TAG SET ODBCABSTRACTION_GIT_TAG = "master"

cmake ..^
    -DARROW_GIT_REPOSITORY=%ARROW_GIT_REPOSITORY%^
    -DARROW_GIT_TAG=%ARROW_GIT_TAG%^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake^
    -DODBCABSTRACTION_REPO=%ODBCABSTRACTION_REPO%^
    -DODBCABSTRACTION_GIT_TAG=%ODBCABSTRACTION_GIT_TAG%^
    -DVCPKG_TARGET_TRIPLET=x86-windows^
    -DVCPKG_MANIFEST_MODE=OFF^
    -G"Visual Studio 17 2022"^
    -A Win32^
    -DCMAKE_BUILD_TYPE=release

cmake --build . --parallel 8 --config Release

cd ..
