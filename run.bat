@echo off
setlocal
cd /d "%~dp0"
title Seam Carving launcher

rem ---- 1. Get the latest code (only if this folder is a git clone and git exists)
where git >nul 2>nul
if %errorlevel%==0 (
    if exist ".git" (
        echo Checking for updates...
        git pull --ff-only
        if errorlevel 1 echo Could not update - continuing with the current version.
    )
) else (
    echo Git not found - skipping update check.
)

rem ---- 2. Find Python
set "PY="
where py >nul 2>nul
if %errorlevel%==0 set "PY=py -3"
if not defined PY (
    where python >nul 2>nul
    if %errorlevel%==0 set "PY=python"
)
if not defined PY (
    echo Python not found. Trying to install it with winget...
    winget install -e --id Python.Python.3.12 --accept-package-agreements --accept-source-agreements
    echo.
    echo If the install finished, CLOSE this window and double-click run.bat again.
    echo Otherwise install Python from https://www.python.org/downloads/ and tick "Add to PATH".
    pause
    exit /b 1
)

rem ---- 3. Private environment so nothing touches the system Python
if not exist ".venv\Scripts\python.exe" (
    echo Creating environment - first run only...
    %PY% -m venv .venv
    if errorlevel 1 goto :fail
)

rem ---- 4. Install libraries - only when requirements.txt is new or changed
fc /b requirements.txt .venv\req.stamp >nul 2>nul
if errorlevel 1 (
    echo Installing libraries...
    ".venv\Scripts\python.exe" -m pip install -q --disable-pip-version-check -r requirements.txt
    if errorlevel 1 goto :fail
    copy /y requirements.txt .venv\req.stamp >nul
)

rem ---- 5. Demo image on first run
if not exist "data\demo.png" ".venv\Scripts\python.exe" scripts\make_demo_image.py

rem ---- 6. Start: no arguments = window app; with arguments = command line (same as main.py)
if "%~1"=="" (
    ".venv\Scripts\python.exe" gui.py
) else (
    ".venv\Scripts\python.exe" main.py %*
)
if errorlevel 1 goto :fail
exit /b 0

:fail
echo.
echo Something went wrong - see the messages above and send them to Isaac.
pause
exit /b 1
