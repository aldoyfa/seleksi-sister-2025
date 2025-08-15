# build.ps1 - Script untuk kompilasi program Mandelbrot di Windows

param(
    [string]$Target = "all"
)

Write-Host "=== Mandelbrot Build Script ===" -ForegroundColor Green

function Test-Command($command) {
    try {
        if (Get-Command $command -ErrorAction Stop) { return $true }
    }
    catch { return $false }
}

function Build-Serial {
    Write-Host "🔨 Compiling Serial version..." -ForegroundColor Yellow
    gcc -O2 -o mandelbrot_serial.exe serial.c
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Serial version compiled successfully!" -ForegroundColor Green
    } else {
        Write-Host "❌ Failed to compile serial version!" -ForegroundColor Red
    }
}

function Build-Parallel {
    Write-Host "🔨 Compiling Parallel version..." -ForegroundColor Yellow
    gcc -fopenmp -O2 -o mandelbrot_parallel.exe parallel.c
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Parallel version compiled successfully!" -ForegroundColor Green
    } else {
        Write-Host "❌ Failed to compile parallel version!" -ForegroundColor Red
    }
}

function Build-GUI {
    Write-Host "🔨 Compiling Windows GUI version..." -ForegroundColor Yellow
    g++ -O2 -std=c++11 -o fractal_gui.exe fractal_gui.cpp -lgdi32 -luser32
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Windows GUI version compiled successfully!" -ForegroundColor Green
    } else {
        Write-Host "❌ Failed to compile Windows GUI version!" -ForegroundColor Red
    }
}

function Build-GPU {
    Write-Host "🔨 Compiling GPU version..." -ForegroundColor Yellow
    
    if (Test-Command "nvcc") {
        nvcc -O2 -Xcompiler -fopenmp -o mandelbrot_gpu.exe gpu.c
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✅ GPU version compiled successfully!" -ForegroundColor Green
        } else {
            Write-Host "❌ Failed to compile GPU version!" -ForegroundColor Red
        }
    } else {
        Write-Host "❌ CUDA compiler (nvcc) not found!" -ForegroundColor Red
        Write-Host "Please install CUDA SDK from: https://developer.nvidia.com/cuda-downloads" -ForegroundColor Yellow
    }
}

function Test-All {
    Write-Host "🧪 Testing compiled programs..." -ForegroundColor Cyan
    
    if (Test-Path "mandelbrot_serial.exe") {
        Write-Host "=== Testing Serial Version ===" -ForegroundColor Blue
        .\mandelbrot_serial.exe
    }
    
    if (Test-Path "mandelbrot_parallel.exe") {
        Write-Host "`n=== Testing Parallel Version ===" -ForegroundColor Blue
        .\mandelbrot_parallel.exe
    }
    
    if (Test-Path "mandelbrot_gpu.exe") {
        Write-Host "`n=== Testing GPU Version ===" -ForegroundColor Blue
        .\mandelbrot_gpu.exe
    }
}

function Clean-Files {
    Write-Host "🧹 Cleaning up..." -ForegroundColor Yellow
    Remove-Item -Path "*.exe", "*.bmp" -ErrorAction SilentlyContinue
    Write-Host "✅ Cleanup completed!" -ForegroundColor Green
}

# Check if GCC is available
if (-not (Test-Command "gcc")) {
    Write-Host "❌ GCC compiler not found!" -ForegroundColor Red
    Write-Host "Please install MinGW-w64 or Visual Studio Build Tools" -ForegroundColor Yellow
    exit 1
}

# Execute based on target
switch ($Target.ToLower()) {
    "serial" { Build-Serial }
    "parallel" { Build-Parallel }
    "gpu" { Build-GPU }
    "all" { 
        Build-Serial
        Build-Parallel 
    }
    "gpu-all" {
        Build-Serial
        Build-Parallel
        Build-GPU
    }
    "gui" { Build-GUI }
    "test" { Test-All }
    "clean" { Clean-Files }
    default {
        Write-Host "Available targets:" -ForegroundColor Cyan
        Write-Host "  serial    - Compile serial version" -ForegroundColor White
        Write-Host "  parallel  - Compile parallel version" -ForegroundColor White
        Write-Host "  gpu       - Compile GPU version (requires CUDA)" -ForegroundColor White
        Write-Host "  all       - Compile serial and parallel" -ForegroundColor White
        Write-Host "  gpu-all   - Compile all versions including GPU" -ForegroundColor White
        Write-Host "  gui       - Compile Windows GUI version" -ForegroundColor White
        Write-Host "  test      - Test all compiled versions" -ForegroundColor White
        Write-Host "  clean     - Remove compiled files" -ForegroundColor White
        Write-Host "`nExample: .\build.ps1 gui" -ForegroundColor Yellow
    }
}
