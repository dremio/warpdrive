@REM Copyright (C) 2020-2022 Dremio Corporation
@REM
@REM See "license.txt" for license information.

@REM Please define ARROW_GIT_REPOSITORY to be the arrow repository. If this is a local repo,
@REM use forward slashes instead of backslashes.

@REM Please define VCPKG_ROOT to be the directory with a built vcpkg. This path should use
@REM forward slashes instead of backslashes.

@ECHO OFF

%VCPKG_ROOT%\vcpkg.exe install --triplet x86-windows --x-install-root=%VCPKG_ROOT%/installed

if exist ".\build" del build /q

mkdir build

cd build

SETLOCAL
if NOT DEFINED ARROW_GIT_REPOSITORY SET ARROW_GIT_REPOSITORY="https://github.com/apache/arrow"
if NOT DEFINED ARROW_GIT_TAG SET ARROW_GIT_TAG="b050bd0d31db6412256cec3362c0d57c9732e1f2"
if NOT DEFINED ODBCABSTRACTION_REPO SET ODBCABSTRACTION_REPO="../flightsql-odbc"
if NOT DEFINED ODBCABSTRACTION_GIT_TAG SET ODBCABSTRACTION_GIT_TAG="7c8b93661c8685fabd630ee467a20ba6ccabc249"

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
