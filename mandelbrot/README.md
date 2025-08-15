# Mandelbrot Set Generator

Implementasi generator himpunan Mandelbrot dengan perbandingan performa antara serial CPU, paralel CPU, dan GPU acceleration.

## 🚀 Hasil Benchmark

### Performa yang Dicapai (pada sistem 12-core):

| Versi | Waktu Eksekusi | Speedup vs Serial |
|-------|---------------|-------------------|
| **Serial (1 thread)** | 1.142 detik | 1.0x (baseline) |
| **Paralel (12 threads)** | 0.258 detik | **4.43x faster** |
| **GPU** | 0.175 detik | **6.53x faster** |

## 🛠️ Cara Kompilasi dan Menjalankan

### Windows (PowerShell):
```powershell
# Kompilasi semua versi command-line
.\build.ps1 all

# Kompilasi GUI interaktif
.\build.ps1 gui

# Kompilasi semua versi GUI
.\build.ps1 gui-all

# Test semua versi
.\build.ps1 test
```

### Linux/Unix:
```bash
# Kompilasi semua versi
make all

# Kompilasi versi GPU (memerlukan CUDA)
make gpu

# Test semua versi
make test
```

### Manual Compilation:
```bash
# Command-line versions
gcc -O2 -o mandelbrot_serial serial.c
gcc -fopenmp -O2 -o mandelbrot_parallel parallel.c
gcc -fopenmp -O2 -o mandelbrot_gpu_sim gpu_simulation.c

# Windows GUI version
g++ -O2 -std=c++11 -o fractal_gui.exe fractal_gui.cpp -lgdi32 -luser32

# GPU version (requires CUDA)
nvcc -O2 -Xcompiler -fopenmp -o mandelbrot_gpu gpu.c
```

# 🎮 Interactive GUI Features

### **Real-Time Fractal Explorer**
- **Interactive Zoom**: Left-click dan drag untuk memilih area zoom
- **Pan Navigation**: Right-click dan drag untuk menggeser pandangan  
- **Mouse Wheel Zoom**: Scroll untuk zoom in/out pada posisi mouse
- **Real-Time Rendering**: Multi-threaded rendering untuk performa optimal

### **Dual Fractal Support**
- **Mandelbrot Set**: Fractal klasik dengan formula z² + c
- **Julia Set**: Konstanta c diambil dari posisi mouse real-time
- **Toggle Mode**: Tekan 'M' untuk beralih antara Mandelbrot dan Julia

### **Advanced Controls**
- **Dynamic Iterations**: +/- untuk mengubah detail dan kualitas
- **Reset View**: 'R' untuk kembali ke pandangan default  
- **Beautiful Colors**: Gradasi warna yang indah berdasarkan iterasi
- **Performance Info**: Waktu rendering dan zoom level ditampilkan

### **GUI Controls:**
```
Left Click + Drag    : Zoom ke area yang dipilih
Right Click + Drag   : Pan (geser) pandangan
Mouse Wheel         : Zoom in/out pada posisi cursor
M                   : Toggle Mandelbrot ↔ Julia Set
R                   : Reset ke pandangan default
+ / -               : Tambah/kurangi iterasi (detail)
Mouse Movement      : Ubah konstanta Julia (mode Julia)
ESC                 : Keluar aplikasi
```

## 🔧 Konfigurasi Program

Parameter yang dapat diubah dalam kode:

```c
// Resolusi gambar
int width = 1920;
int height = 1080;

// Detail dan kualitas
int max_iterations = 1000;

// Area kompleks yang digambar
double min_real = -2.5;
double max_real = 1.0;
double min_imag = -1.0;
double max_imag = 1.0;
```