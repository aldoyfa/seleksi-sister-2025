#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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
    
    // Alokasi memori untuk gambar
    RGB* image = (RGB*)malloc(width * height * sizeof(RGB));
    if (!image) return 1;
    
    // Hitung skala untuk konversi dari koordinat pixel ke koordinat kompleks
    double real_scale = (max_real - min_real) / width;
    double imag_scale = (max_imag - min_imag) / height;
    
    // Generate gambar Mandelbrot
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Konversi koordinat pixel ke koordinat kompleks
            double real = min_real + x * real_scale;
            double imag = min_imag + y * imag_scale;
            
            // Hitung iterasi Mandelbrot
            int iterations = mandelbrot_iterations(real, imag, max_iterations);
            
            // Konversi ke warna
            image[y * width + x] = get_color(iterations, max_iterations);
        }
    }
    
    // Simpan gambar
    if (!save_bmp("mandelbrot.bmp", image, width, height)) {
        free(image);
        return 1;
    }
    
    // Bersihkan memori
    free(image);
    
    return 0;
}   