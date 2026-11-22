@echo off
cd /d "%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VSPATH=%%i"

if defined VSPATH if exist "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" (
    call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
    cl /nologo /EHsc /std:c++17 /O2 omnicalc.cpp
    del omnicalc.obj 2>nul
    goto done
)

where g++ >nul 2>nul
if %errorlevel%==0 (
    g++ -std=c++17 -O2 omnicalc.cpp -o omnicalc.exe -mwindows
    goto done
)

echo No C++ compiler found. Install Visual Studio Build Tools or MinGW g++.
pause
exit /b 1

:done
if exist omnicalc.exe (
    echo Build finished: omnicalc.exe
    start "" omnicalc.exe
) else (
    echo Build failed.
    pause
)
