@echo off
echo Compiling NTT-based big integer multiplication program...
gcc program.c -O2 -std=c11 -o program.exe
if %errorlevel% equ 0 (
    echo Compilation successful! program.exe created.
) else (
    echo Compilation failed with error code %errorlevel%
)
pause
