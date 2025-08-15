#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
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

// Fungsi untuk menghitung iterasi Mandelbrot
int mandelbrot_iterations(double real, double imag, int max_iter) {
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

// Fungsi untuk mengkonversi iterasi ke warna
RGB get_color(int iterations, int max_iter) {
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

// Fungsi untuk menyimpan gambar BMP
int save_bmp(const char* filename, RGB* image, int width, int height) {
    FILE* file = fopen(filename, "wb");
    if (!file) return 0;
    
    // Hitung padding untuk setiap baris (BMP memerlukan padding ke kelipatan 4 byte)
    int padding = (4 - (width * 3) % 4) % 4;
    int row_size = width * 3 + padding;
    
    // Header BMP
    BMPHeader header;
    header.type = 0x4D42; // "BM"
    header.size = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + row_size * height;
    header.reserved1 = 0;
    header.reserved2 = 0;
    header.offset = sizeof(BMPHeader) + sizeof(BMPInfoHeader);
    
    // Info header BMP
    BMPInfoHeader info;
    info.size = sizeof(BMPInfoHeader);
    info.width = width;
    info.height = height;
    info.planes = 1;
    info.bits_per_pixel = 24;
    info.compression = 0;
    info.image_size = row_size * height;
    info.x_pixels_per_meter = 2835; // 72 DPI
    info.y_pixels_per_meter = 2835;
    info.colors_used = 0;
    info.colors_important = 0;
    
    // Tulis header
    fwrite(&header, sizeof(BMPHeader), 1, file);
    fwrite(&info, sizeof(BMPInfoHeader), 1, file);
    
    // Tulis data pixel (BMP menyimpan dari bawah ke atas)
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

// Versi serial untuk rendering Mandelbrot
void render_mandelbrot_serial(RGB* image, int width, int height, int max_iterations,
                             double min_real, double max_real, double min_imag, double max_imag) {
    double real_scale = (max_real - min_real) / width;
    double imag_scale = (max_imag - min_imag) / height;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double real = min_real + x * real_scale;
            double imag = min_imag + y * imag_scale;
            
            int iterations = mandelbrot_iterations(real, imag, max_iterations);
            image[y * width + x] = get_color(iterations, max_iterations);
        }
    }
}

// Versi paralel untuk rendering Mandelbrot menggunakan OpenMP
void render_mandelbrot_parallel(RGB* image, int width, int height, int max_iterations,
                               double min_real, double max_real, double min_imag, double max_imag) {
    double real_scale = (max_real - min_real) / width;
    double imag_scale = (max_imag - min_imag) / height;
    
    // Paralelisasi loop berdasarkan baris (y)
    // Setiap thread akan mengerjakan beberapa baris
    #pragma omp parallel for schedule(dynamic, 1)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double real = min_real + x * real_scale;
            double imag = min_imag + y * imag_scale;
            
            int iterations = mandelbrot_iterations(real, imag, max_iterations);
            image[y * width + x] = get_color(iterations, max_iterations);
        }
    }
}

// Fungsi untuk mengukur waktu
double get_time() {
    return omp_get_wtime();
}

int main() {
    // Parameter yang bisa diubah
    int width = 1920;
    int height = 1080;
    int max_iterations = 1000;
    
    // Batas area kompleks yang akan digambar
    double min_real = -2.5;
    double max_real = 1.0;
    double min_imag = -1.0;
    double max_imag = 1.0;
    
    printf("=== BENCHMARK MANDELBROT SET RENDERING ===\n");
    printf("Resolusi: %dx%d pixels\n", width, height);
    printf("Max iterasi: %d\n", max_iterations);
    printf("Jumlah thread tersedia: %d\n", omp_get_max_threads());
    printf("\n");
    
    // Alokasi memori untuk gambar
    RGB* image_serial = (RGB*)malloc(width * height * sizeof(RGB));
    RGB* image_parallel = (RGB*)malloc(width * height * sizeof(RGB));
    
    if (!image_serial || !image_parallel) {
        printf("Error: Gagal mengalokasi memori\n");
        free(image_serial);
        free(image_parallel);
        return 1;
    }
    
    // === BENCHMARK VERSI SERIAL ===
    printf("Menjalankan versi SERIAL...\n");
    double start_serial = get_time();
    
    render_mandelbrot_serial(image_serial, width, height, max_iterations,
                           min_real, max_real, min_imag, max_imag);
    
    double end_serial = get_time();
    double time_serial = end_serial - start_serial;
    
    printf("Waktu serial: %.3f detik\n", time_serial);
    
    // Simpan hasil serial
    if (!save_bmp("mandelbrot_serial.bmp", image_serial, width, height)) {
        printf("Error: Gagal menyimpan gambar serial\n");
    } else {
        printf("Gambar serial disimpan: mandelbrot_serial.bmp\n");
    }
    
    printf("\n");
    
    // === BENCHMARK VERSI PARALEL ===
    printf("Menjalankan versi PARALEL...\n");
    double start_parallel = get_time();
    
    render_mandelbrot_parallel(image_parallel, width, height, max_iterations,
                              min_real, max_real, min_imag, max_imag);
    
    double end_parallel = get_time();
    double time_parallel = end_parallel - start_parallel;
    
    printf("Waktu paralel: %.3f detik\n", time_parallel);
    
    // Simpan hasil paralel
    if (!save_bmp("mandelbrot_parallel.bmp", image_parallel, width, height)) {
        printf("Error: Gagal menyimpan gambar paralel\n");
    } else {
        printf("Gambar paralel disimpan: mandelbrot_parallel.bmp\n");
    }
    
    printf("\n");
    
    // === ANALISIS PERFORMA ===
    double speedup = time_serial / time_parallel;
    double efficiency = speedup / omp_get_max_threads() * 100;
    
    printf("=== HASIL BENCHMARK ===\n");
    printf("Waktu serial:    %.3f detik\n", time_serial);
    printf("Waktu paralel:   %.3f detik\n", time_parallel);
    printf("Speedup:         %.2fx\n", speedup);
    printf("Efisiensi:       %.1f%%\n", efficiency);
    printf("Thread digunakan: %d\n", omp_get_max_threads());
    
    if (speedup > 1.0) {
        printf("✓ Paralelisasi berhasil mempercepat proses!\n");
    } else {
        printf("⚠ Paralelisasi tidak memberikan percepatan.\n");
    }
    
    // Verifikasi bahwa kedua hasil identik
    int identical = 1;
    for (int i = 0; i < width * height && identical; i++) {
        if (image_serial[i].r != image_parallel[i].r ||
            image_serial[i].g != image_parallel[i].g ||
            image_serial[i].b != image_parallel[i].b) {
            identical = 0;
        }
    }
    
    if (identical) {
        printf("✓ Verifikasi: Hasil serial dan paralel identik\n");
    } else {
        printf("⚠ Peringatan: Hasil serial dan paralel berbeda\n");
    }
    
    // Bersihkan memori
    free(image_serial);
    free(image_parallel);
    
    return 0;
}
