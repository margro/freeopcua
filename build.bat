@echo off

SET BOOST_VERSION=1_78_0
SET BOOST_ROOT=C:\local\boost_%BOOST_VERSION%

if defined xxsim_toolchain (
	SET BOOST_ROOT=%xxsim_toolchain%\boost_%BOOST_VERSION%
)

set CURDIR=%CD%
if not exist "%~dp0\build" (
	mkdir build
)
cd "%~dp0\build

cmake ..\ -G "Visual Studio 17 2022" -A win32 -DBOOST_ROOT=%BOOST_ROOT% -DSSL_SUPPORT_MBEDTLS=off -DBUILD_SHARED_LIBS=off -DBoost_USE_STATIC_LIBS=ON -DBoost_USE_STATIC_RUNTIME=ON
cd %CURDIR%
