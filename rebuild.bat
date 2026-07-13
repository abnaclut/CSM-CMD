@echo off
setlocal enabledelayedexpansion

echo ========================================
echo    csm_terminal - Rebuild Script
echo ========================================
echo.

set BUILD_DIR=build
set CONFIG=Release
set TESTS=ON

if /i "%~1"=="--debug" set CONFIG=Debug
if /i "%~1"=="--no-tests" set TESTS=OFF
if /i "%~2"=="--debug" set CONFIG=Debug
if /i "%~2"=="--no-tests" set TESTS=OFF

if exist "%BUILD_DIR%" (
    echo [1/5] Removing old build directory...
    rmdir /s /q "%BUILD_DIR%"
    if errorlevel 1 (
        echo [ERROR] Failed to remove build directory
        pause
        exit /b 1
    )
)

echo [2/5] Creating fresh build directory...
mkdir "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to create build directory
    pause
    exit /b 1
)

cd "%BUILD_DIR%"

echo [3/5] Running CMake with Ninja...
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=%CONFIG% -DCMAKE_CXX_FLAGS="-mconsole" -DBUILD_TERMINAL_TESTS=%TESTS%
if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    cd ..
    pause
    exit /b 1
)

echo [4/5] Building project with Ninja...
ninja csm_terminal
if errorlevel 1 (
    echo [ERROR] Build failed
    cd ..
    pause
    exit /b 1
)

if /i "%TESTS%"=="ON" (
    echo [5/5] Building and running tests...
    ninja csm_cmd_tests
    if errorlevel 1 (
        echo [ERROR] Test build failed
        cd ..
        pause
        exit /b 1
    )
    ctest --output-on-failure
    if errorlevel 1 (
        echo [WARNING] Some tests failed
    )
) else (
    echo [5/5] Skipping tests
)

cd ..

echo.
echo ========================================
echo    Build successful!
echo ========================================
echo.
set EXE_PATH=%BUILD_DIR%\bin\csm_terminal.exe
if not exist "%EXE_PATH%" (
    echo [WARNING] Executable not found at expected location.
    echo Trying alternative location...
    set EXE_PATH=%BUILD_DIR%\csm_terminal.exe
)

echo Executable: %EXE_PATH%
echo.
echo To run: %EXE_PATH% -nogui
echo.
pause