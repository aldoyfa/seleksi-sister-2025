#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <cuda_runtime.h>
#include <omp.h>

// Struktur untuk header BMP
#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} BMPInfoHeader;
#pragma pack(pop)

// Struktur untuk warna RGB
typedef struct {
    uint8_t b, g, r;
} RGB;

// CUDA kernel untuk menghitung iterasi Mandelbrot
__device__ int mandelbrot_iterations_gpu(double real, double imag, int max_iter) {
    double z_real = 0.0;
    double z_imag = 0.0;
    int iter = 0;
    
    while (iter < max_iter && (z_real * z_real + z_imag * z_imag) < 4.0) {
        double temp = z_real * z_real - z_imag * z_imag + real;
        z_imag = 2.0 * z_real * z_imag + imag;
        z_real = temp;
        iter++;
    }
    
    return iter;
}

// CUDA kernel untuk mengkonversi iterasi ke warna
__device__ RGB get_color_gpu(int iterations, int max_iter) {
    RGB color;
    
    if (iterations == max_iter) {
        // Titik dalam himpunan Mandelbrot (hitam)
        color.r = 0;
        color.g = 0;
        color.b = 0;
    } else {
        // Gradasi warna berdasarkan iterasi
        double ratio = (double)iterations / max_iter;
        
        // Skema warna biru ke merah
        if (ratio < 0.5) {
            color.r = (uint8_t)(255 * ratio * 2);
            color.g = 0;
            color.b = (uint8_t)(255 * (1 - ratio * 2));
        } else {
            color.r = 255;
            color.g = (uint8_t)(255 * (ratio - 0.5) * 2);
            color.b = 0;
        }
    }
    
    return color;
}

// CUDA kernel untuk rendering Mandelbrot
__global__ void mandelbrot_kernel(RGB* image, int width, int height, int max_iterations,
                                 double min_real, double max_real, double min_imag, double max_imag) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (x < width && y < height) {
        // Hitung skala untuk konversi dari koordinat pixel ke koordinat kompleks
        double real_scale = (max_real - min_real) / width;
        double imag_scale = (max_imag - min_imag) / height;
        
        // Konversi koordinat pixel ke koordinat kompleks
        double real = min_real + x * real_scale;
        double imag = min_imag + y * imag_scale;
        
        // Hitung iterasi Mandelbrot
        int iterations = mandelbrot_iterations_gpu(real, imag, max_iterations);
        
        // Konversi ke warna dan simpan
        image[y * width + x] = get_color_gpu(iterations, max_iterations);
    }
}

// Versi CPU untuk perbandingan
int mandelbrot_iterations_cpu(double real, double imag, int max_iter) {
    double z_real = 0.0;
    double z_imag = 0.0;
    int iter = 0;
    
    while (iter < max_iter && (z_real * z_real + z_imag * z_imag) < 4.0) {
        double temp = z_real * z_real - z_imag * z_imag + real;
        z_imag = 2.0 * z_real * z_imag + imag;
        z_real = temp;
        iter++;
    }
    
    return iter;
}

RGB get_color_cpu(int iterations, int max_iter) {
    RGB color;
    
    if (iterations == max_iter) {
        color.r = 0;
        color.g = 0;
        color.b = 0;
    } else {
        double ratio = (double)iterations / max_iter;
        
        if (ratio < 0.5) {
            color.r = (uint8_t)(255 * ratio * 2);
            color.g = 0;
            color.b = (uint8_t)(255 * (1 - ratio * 2));
        } else {
            color.r = 255;
            color.g = (uint8_t)(255 * (ratio - 0.5) * 2);
            color.b = 0;
        }
    }
    
    return color;
}

// Versi serial CPU
void render_mandelbrot_serial(RGB* image, int width, int height, int max_iterations,
                             double min_real, double max_real, double min_imag, double max_imag) {
    double real_scale = (max_real - min_real) / width;
    double imag_scale = (max_imag - min_imag) / height;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double real = min_real + x * real_scale;
            double imag = min_imag + y * imag_scale;
            
            int iterations = mandelbrot_iterations_cpu(real, imag, max_iterations);
            image[y * width + x] = get_color_cpu(iterations, max_iterations);
        }
    }
}

// Versi paralel CPU dengan OpenMP
void render_mandelbrot_parallel(RGB* image, int width, int height, int max_iterations,
                               double min_real, double max_real, double min_imag, double max_imag) {
    double real_scale = (max_real - min_real) / width;
    double imag_scale = (max_imag - min_imag) / height;
    
    #pragma omp parallel for schedule(dynamic, 1)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double real = min_real + x * real_scale;
            double imag = min_imag + y * imag_scale;
            
            int iterations = mandelbrot_iterations_cpu(real, imag, max_iterations);
            image[y * width + x] = get_color_cpu(iterations, max_iterations);
        }
    }
}

// Versi GPU dengan CUDA
void render_mandelbrot_gpu(RGB* image, int width, int height, int max_iterations,
                          double min_real, double max_real, double min_imag, double max_imag) {
    RGB* d_image;
    size_t image_size = width * height * sizeof(RGB);
    
    // Alokasi memori di GPU
    cudaMalloc(&d_image, image_size);
    
    // Setup grid dan block dimensions
    dim3 block_size(16, 16);
    dim3 grid_size((width + block_size.x - 1) / block_size.x,
                   (height + block_size.y - 1) / block_size.y);
    
    // Launch kernel
    mandelbrot_kernel<<<grid_size, block_size>>>(d_image, width, height, max_iterations,
                                                min_real, max_real, min_imag, max_imag);
    
    // Wait for completion
    cudaDeviceSynchronize();
    
    // Copy hasil kembali ke host
    cudaMemcpy(image, d_image, image_size, cudaMemcpyDeviceToHost);
    
    // Bersihkan memori GPU
    cudaFree(d_image);
}

// Fungsi untuk menyimpan gambar BMP
int save_bmp(const char* filename, RGB* image, int width, int height) {
    FILE* file = fopen(filename, "wb");
    if (!file) return 0;
    
    int padding = (4 - (width * 3) % 4) % 4;
    int row_size = width * 3 + padding;
    
    BMPHeader header;
    header.type = 0x4D42;
    header.size = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + row_size * height;
    header.reserved1 = 0;
    header.reserved2 = 0;
    header.offset = sizeof(BMPHeader) + sizeof(BMPInfoHeader);
    
    BMPInfoHeader info;
    info.size = sizeof(BMPInfoHeader);
    info.width = width;
    info.height = height;
    info.planes = 1;
    info.bits_per_pixel = 24;
    info.compression = 0;
    info.image_size = row_size * height;
    info.x_pixels_per_meter = 2835;
    info.y_pixels_per_meter = 2835;
    info.colors_used = 0;
    info.colors_important = 0;
    
    fwrite(&header, sizeof(BMPHeader), 1, file);
    fwrite(&info, sizeof(BMPInfoHeader), 1, file);
    
    uint8_t padding_bytes[3] = {0, 0, 0};
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            RGB pixel = image[y * width + x];
            fwrite(&pixel, sizeof(RGB), 1, file);
        }
        if (padding > 0) {
            fwrite(padding_bytes, padding, 1, file);
        }
    }
    
    fclose(file);
    return 1;
}

// Fungsi untuk cek CUDA device info
void print_cuda_device_info() {
    int device_count = 0;
    cudaGetDeviceCount(&device_count);
    
    if (device_count == 0) {
        printf("Tidak ada CUDA device yang ditemukan!\n");
        return;
    }
    
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    
    printf("=== CUDA DEVICE INFO ===\n");
    printf("Device: %s\n", prop.name);
    printf("Compute capability: %d.%d\n", prop.major, prop.minor);
    printf("Global memory: %.1f GB\n", prop.totalGlobalMem / 1024.0 / 1024.0 / 1024.0);
    printf("Multiprocessors: %d\n", prop.multiProcessorCount);
    printf("Max threads per block: %d\n", prop.maxThreadsPerBlock);
    printf("Warp size: %d\n", prop.warpSize);
    printf("\n");
}

double get_time() {
    return omp_get_wtime();
}

int main() {
    // Parameter
    int width = 1920;
    int height = 1080;
    int max_iterations = 1000;
    
    double min_real = -2.5;
    double max_real = 1.0;
    double min_imag = -1.0;
    double max_imag = 1.0;
    
    printf("=== MANDELBROT GPU ACCELERATION BENCHMARK ===\n");
    printf("Resolusi: %dx%d pixels\n", width, height);
    printf("Max iterasi: %d\n", max_iterations);
    printf("CPU threads tersedia: %d\n", omp_get_max_threads());
    printf("\n");
    
    // Print CUDA device info
    print_cuda_device_info();
    
    // Alokasi memori untuk gambar
    RGB* image_serial = (RGB*)malloc(width * height * sizeof(RGB));
    RGB* image_parallel = (RGB*)malloc(width * height * sizeof(RGB));
    RGB* image_gpu = (RGB*)malloc(width * height * sizeof(RGB));
    
    if (!image_serial || !image_parallel || !image_gpu) {
        printf("Error: Gagal mengalokasi memori\n");
        free(image_serial);
        free(image_parallel);
        free(image_gpu);
        return 1;
    }
    
    // === BENCHMARK VERSI SERIAL ===
    printf("🔄 Menjalankan versi SERIAL (CPU single-thread)...\n");
    double start_serial = get_time();
    
    render_mandelbrot_serial(image_serial, width, height, max_iterations,
                           min_real, max_real, min_imag, max_imag);
    
    double end_serial = get_time();
    double time_serial = end_serial - start_serial;
    
    printf("✅ Waktu serial: %.3f detik\n", time_serial);
    save_bmp("mandelbrot_serial.bmp", image_serial, width, height);
    printf("📁 Gambar serial disimpan: mandelbrot_serial.bmp\n\n");
    
    // === BENCHMARK VERSI PARALEL CPU ===
    printf("🔄 Menjalankan versi PARALEL (CPU multi-thread)...\n");
    double start_parallel = get_time();
    
    render_mandelbrot_parallel(image_parallel, width, height, max_iterations,
                              min_real, max_real, min_imag, max_imag);
    
    double end_parallel = get_time();
    double time_parallel = end_parallel - start_parallel;
    
    printf("✅ Waktu paralel: %.3f detik\n", time_parallel);
    save_bmp("mandelbrot_parallel.bmp", image_parallel, width, height);
    printf("📁 Gambar paralel disimpan: mandelbrot_parallel.bmp\n\n");
    
    // === BENCHMARK VERSI GPU ===
    printf("🔄 Menjalankan versi GPU (CUDA)...\n");
    double start_gpu = get_time();
    
    render_mandelbrot_gpu(image_gpu, width, height, max_iterations,
                         min_real, max_real, min_imag, max_imag);
    
    double end_gpu = get_time();
    double time_gpu = end_gpu - start_gpu;
    
    printf("✅ Waktu GPU: %.3f detik\n", time_gpu);
    save_bmp("mandelbrot_gpu.bmp", image_gpu, width, height);
    printf("📁 Gambar GPU disimpan: mandelbrot_gpu.bmp\n\n");
    
    // === ANALISIS PERFORMA ===
    double speedup_parallel = time_serial / time_parallel;
    double speedup_gpu = time_serial / time_gpu;
    double gpu_vs_parallel = time_parallel / time_gpu;
    
    printf("=== 📊 HASIL BENCHMARK LENGKAP ===\n");
    printf("🐌 Serial (1 thread):    %.3f detik\n", time_serial);
    printf("⚡ Paralel (%d threads):  %.3f detik\n", omp_get_max_threads(), time_parallel);
    printf("🚀 GPU (CUDA):           %.3f detik\n", time_gpu);
    printf("\n");
    printf("📈 SPEEDUP ANALYSIS:\n");
    printf("CPU Paralel vs Serial:   %.2fx faster\n", speedup_parallel);
    printf("GPU vs Serial:           %.2fx faster\n", speedup_gpu);
    printf("GPU vs CPU Paralel:      %.2fx faster\n", gpu_vs_parallel);
    printf("\n");
    
    // Analisis efisiensi
    double cpu_efficiency = speedup_parallel / omp_get_max_threads() * 100;
    printf("🎯 EFISIENSI:\n");
    printf("CPU Paralel Efficiency:  %.1f%%\n", cpu_efficiency);
    
    if (speedup_gpu > speedup_parallel) {
        printf("🏆 GPU memberikan performa terbaik!\n");
    } else if (speedup_parallel > 1.0) {
        printf("🥈 CPU Paralel memberikan performa terbaik!\n");
    } else {
        printf("🐌 Serial masih yang tercepat (tidak normal).\n");
    }
    
    // Verifikasi hasil identik
    int identical_parallel = 1, identical_gpu = 1;
    for (int i = 0; i < width * height; i++) {
        if (image_serial[i].r != image_parallel[i].r ||
            image_serial[i].g != image_parallel[i].g ||
            image_serial[i].b != image_parallel[i].b) {
            identical_parallel = 0;
            break;
        }
    }
    
    for (int i = 0; i < width * height; i++) {
        if (image_serial[i].r != image_gpu[i].r ||
            image_serial[i].g != image_gpu[i].g ||
            image_serial[i].b != image_gpu[i].b) {
            identical_gpu = 0;
            break;
        }
    }
    
    printf("\n🔍 VERIFIKASI HASIL:\n");
    printf("Serial vs Paralel: %s\n", identical_parallel ? "✅ Identik" : "❌ Berbeda");
    printf("Serial vs GPU:     %s\n", identical_gpu ? "✅ Identik" : "❌ Berbeda");
    
    // Bersihkan memori
    free(image_serial);
    free(image_parallel);
    free(image_gpu);
    
    return 0;
}
