@echo off
echo Building PixLegends...

REM Create build directory if it doesn't exist
if not exist "build" mkdir build

REM Change to build directory
cd build

REM Configure with CMake (use Visual Studio 2022 generator)
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build the project
echo Building project...
cmake --build . --config Release

REM Check if build was successful
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Executable created in: %CD%\Release\PixLegends.exe
    echo.
    echo To run the game, execute: Release\PixLegends.exe
) else (
    echo Build failed!
    pause
)

cd ..
