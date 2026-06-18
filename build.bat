@echo off
setlocal

REM =====================================================
REM  WSL Tool Build Script
REM  Qt 5.15.2 MinGW 8.1.0 x64
REM =====================================================

set QT_DIR=C:\Qt\5.15.2\mingw81_64
set MINGW_DIR=C:\Qt\Tools\mingw810_64
set PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%

set BUILD_DIR=%~dp0build\release
set SOURCE_DIR=%~dp0

echo ============================================
echo   WSL Tool Build Script
echo   Qt: %QT_DIR%
echo   MinGW: %MINGW_DIR%
echo ============================================

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo.
echo [1/4] Running qmake...
cd /d "%BUILD_DIR%"
qmake "%SOURCE_DIR%WSLTool.pro" -spec win32-g++ "CONFIG+=release" "CONFIG-=debug" || goto :error

echo.
echo [2/4] Compiling (mingw32-make)...
mingw32-make -j%NUMBER_OF_PROCESSORS% || goto :error

echo.
echo [3/4] Running windeployqt...
if exist "%BUILD_DIR%\WSLTool.exe" (
    echo Running windeployqt in release mode...
    windeployqt --release --no-translations "%BUILD_DIR%\WSLTool.exe"
    if errorlevel 1 (
        echo [WARNING] windeployqt --release failed, trying with --debug mode...
        windeployqt --debug --no-translations "%BUILD_DIR%\WSLTool.exe" || goto :error
    )
    echo windeployqt completed.
) else (
    echo [ERROR] WSLTool.exe not found.
    goto :error
)

echo.
echo [4/4] Build completed successfully!
echo Output directory: %BUILD_DIR%
echo.
dir "%BUILD_DIR%\WSLTool.exe"
echo.
echo Run application: %BUILD_DIR%\WSLTool.exe
goto :end

:error
echo.
echo [ERROR] Build failed! Please check error output.
pause
exit /b 1

:end
endlocal
pause
