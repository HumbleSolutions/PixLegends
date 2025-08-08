@echo off
echo Setting up SDL2 development libraries for PixLegends...
echo.

REM Create external directory
if not exist "external" mkdir external
cd external

REM Download SDL2 development libraries
echo Downloading SDL2 development libraries...

REM SDL2
if not exist "SDL2-devel-2.28.5-VC.zip" (
    echo Downloading SDL2...
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-devel-2.28.5-VC.zip' -OutFile 'SDL2-devel-2.28.5-VC.zip'"
)

REM SDL2_image
if not exist "SDL2_image-devel-2.6.3-VC.zip" (
    echo Downloading SDL2_image...
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/libsdl-org/SDL_image/releases/download/release-2.6.3/SDL2_image-devel-2.6.3-VC.zip' -OutFile 'SDL2_image-devel-2.6.3-VC.zip'"
)

REM SDL2_ttf
if not exist "SDL2_ttf-devel-2.20.2-VC.zip" (
    echo Downloading SDL2_ttf...
    powershell -Command "Invoke-WebRequest -Uri 'https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.20.2/SDL2_ttf-devel-2.20.2-VC.zip' -OutFile 'SDL2_ttf-devel-2.20.2-VC.zip'"
)

echo.
echo Extracting SDL2 libraries...

REM Extract SDL2
if not exist "SDL2-2.28.5" (
    echo Extracting SDL2...
    powershell -Command "Expand-Archive -Path 'SDL2-devel-2.28.5-VC.zip' -DestinationPath '.' -Force"
)

REM Extract SDL2_image
if not exist "SDL2_image-2.6.3" (
    echo Extracting SDL2_image...
    powershell -Command "Expand-Archive -Path 'SDL2_image-devel-2.6.3-VC.zip' -DestinationPath '.' -Force"
)

REM Extract SDL2_ttf
if not exist "SDL2_ttf-2.20.2" (
    echo Extracting SDL2_ttf...
    powershell -Command "Expand-Archive -Path 'SDL2_ttf-devel-2.20.2-VC.zip' -DestinationPath '.' -Force"
)

echo.
echo Setting up SDL2 directory structure...

REM Create SDL2 directory
if not exist "SDL2" mkdir SDL2
if not exist "SDL2\include" mkdir SDL2\include
if not exist "SDL2\lib" mkdir SDL2\lib
if not exist "SDL2\lib\x64" mkdir SDL2\lib\x64

REM Copy SDL2 files
echo Copying SDL2 files...
xcopy "SDL2-2.28.5\include\*" "SDL2\include\" /E /I /Y
xcopy "SDL2-2.28.5\lib\x64\*" "SDL2\lib\x64\" /E /I /Y

REM Copy SDL2_image files
echo Copying SDL2_image files...
xcopy "SDL2_image-2.6.3\include\*" "SDL2\include\" /E /I /Y
xcopy "SDL2_image-2.6.3\lib\x64\*" "SDL2\lib\x64\" /E /I /Y

REM Copy SDL2_ttf files
echo Copying SDL2_ttf files...
xcopy "SDL2_ttf-2.20.2\include\*" "SDL2\include\" /E /I /Y
xcopy "SDL2_ttf-2.20.2\lib\x64\*" "SDL2\lib\x64\" /E /I /Y

cd ..

echo.
echo SDL2 setup complete! You can now run build.bat to build the project.
echo.
pause
