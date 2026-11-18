@echo off
setlocal
cd /d "%~dp0"

if not defined OpenCV_DIR if exist "build\deps\opencv\build\OpenCVConfig.cmake" set "OpenCV_DIR=%CD%\build\deps\opencv\build"
if not defined OpenCV_DIR if exist "C:\opencv\build\OpenCVConfig.cmake" set "OpenCV_DIR=C:\opencv\build"

cmake -S . -B build/native || goto :fail
cmake --build build/native --config Release --target seam-carve || goto :fail

rem Load DLLs from the SDK selected by CMake, including a cached custom SDK.
for /f "tokens=1,* delims==" %%A in ('findstr /b /c:"OpenCV_DIR:" "build\native\CMakeCache.txt"') do (
    if exist "%%B\..\bin\opencv*.dll" set "PATH=%%B\..\bin;%PATH%"
    for /d %%D in ("%%B\x64\vc*") do if exist "%%~D\bin\opencv*.dll" set "PATH=%%~D\bin;%PATH%"
)

set "app=build\native\Release\seam-carve.exe"
if not exist "%app%" set "app=build\native\seam-carve.exe"
"%app%" %* || goto :fail
exit /b 0

:fail
echo Check the C++ and OpenCV setup in README.txt.
if "%~1"=="" pause
exit /b 1
