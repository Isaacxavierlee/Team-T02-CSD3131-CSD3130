@echo off
setlocal
cd /d "%~dp0"

if not defined OpenCV_DIR if exist "build\deps\opencv\build\OpenCVConfig.cmake" set "OpenCV_DIR=%CD%\build\deps\opencv\build"
if defined OpenCV_DIR (
    for /d %%D in ("%OpenCV_DIR%\x64\vc*") do if exist "%%~D\bin" set "PATH=%%~D\bin;%PATH%"
)

cmake -S . -B build/native
if errorlevel 1 goto :fail
cmake --build build/native --config Release --target seam-carve
if errorlevel 1 goto :fail

set "app=build\native\Release\seam-carve.exe"
if not exist "%app%" set "app=build\native\seam-carve.exe"
"%app%" %*
if errorlevel 1 goto :fail
exit /b 0

:fail
echo Check the C++ and OpenCV setup in README.md.
if "%~1"=="" pause
exit /b 1
