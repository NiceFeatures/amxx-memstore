@echo off
setlocal

echo [MemStore] Configuring CMake (32-bit Win32)...
cmake -B build -A Win32 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [ERROR] CMake configuration failed!
    exit /b %errorlevel%
)

echo [MemStore] Compiling Release build...
cmake --build build --config Release
if errorlevel 1 (
    echo [ERROR] Compilation failed!
    exit /b %errorlevel%
)

echo [SUCCESS] Binary compiled at: build\Release\memstore_amxx.dll
pause
