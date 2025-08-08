@echo off
echo Setting up fonts for PixLegends...
echo.

REM Create fonts directory if it doesn't exist
if not exist "assets\fonts" mkdir assets\fonts

REM Download a free font (Liberation Sans)
echo Downloading Liberation Sans font...
powershell -Command "Invoke-WebRequest -Uri 'https://github.com/liberationfonts/liberation-fonts/raw/main/liberation-fonts-ttf-2.1.5/LiberationSans-Regular.ttf' -OutFile 'assets\fonts\arial.ttf'"

if exist "assets\fonts\arial.ttf" (
    echo Font downloaded successfully!
) else (
    echo Failed to download font. Creating a simple fallback...
    echo You may need to manually download a TTF font and place it as 'assets\fonts\arial.ttf'
)

echo.
echo Font setup complete!
pause
