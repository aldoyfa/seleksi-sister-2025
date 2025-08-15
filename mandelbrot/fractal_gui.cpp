#include <iostream>
#include <vector>
#include <complex>
#include <thread>
#include <chrono>
#include <atomic>

// Simple CPU-only version without external dependencies
// Uses Windows API for basic window and graphics

#ifdef _WIN32
#include <windows.h>
#include <wingdi.h>

class SimpleFractalViewer {
private:
    HWND hwnd;
    HDC hdc;
    HBITMAP hBitmap;
    void* bitmapData;
    
    int width, height;
    int max_iterations;
    double zoom;
    double center_real, center_imag;
    bool is_julia;
    std::complex<double> julia_c;
    
    std::atomic<bool> is_rendering;
    bool is_dragging, is_selecting;
    POINT drag_start, selection_start, selection_end;
    
    std::vector<COLORREF> pixels;
    
public:
    SimpleFractalViewer(int w, int h) 
        : width(w), height(h), max_iterations(100), zoom(1.0),
          center_real(-0.5), center_imag(0.0), is_julia(false),
          julia_c(0.3, 0.5), is_rendering(false), is_dragging(false), is_selecting(false) {
        
        pixels.resize(width * height);
    }
    
    // Mandelbrot calculation
    int mandelbrot_iterations(std::complex<double> c) {
        std::complex<double> z(0, 0);
        int iter = 0;
        
        while (iter < max_iterations && std::abs(z) < 2.0) {
            z = z * z + c;
            iter++;
        }
        
        return iter;
    }
    
    // Julia calculation
    int julia_iterations(std::complex<double> z) {
        int iter = 0;
        
        while (iter < max_iterations && std::abs(z) < 2.0) {
            z = z * z + julia_c;
            iter++;
        }
        
        return iter;
    }
    
    // Convert iterations to color
    COLORREF get_color(int iterations) {
        if (iterations == max_iterations) {
            return RGB(0, 0, 0); // Black
        }
        
        double ratio = static_cast<double>(iterations) / max_iterations;
        
        int r, g, b;
        
        if (ratio < 0.16) {
            r = static_cast<int>(255 * ratio * 6);
            g = 0;
            b = static_cast<int>(255 * (1 - ratio * 6));
        } else if (ratio < 0.33) {
            double r_ratio = (ratio - 0.16) * 6;
            r = 255;
            g = static_cast<int>(255 * r_ratio);
            b = 0;
        } else if (ratio < 0.5) {
            double r_ratio = (ratio - 0.33) * 6;
            r = 255;
            g = 255;
            b = static_cast<int>(255 * r_ratio);
        } else if (ratio < 0.66) {
            double r_ratio = (ratio - 0.5) * 6;
            r = static_cast<int>(255 * (1 - r_ratio));
            g = 255;
            b = 255;
        } else if (ratio < 0.83) {
            double r_ratio = (ratio - 0.66) * 6;
            r = 0;
            g = static_cast<int>(255 * (1 - r_ratio));
            b = 255;
        } else {
            double r_ratio = (ratio - 0.83) * 6;
            r = static_cast<int>(255 * r_ratio);
            g = 0;
            b = 255;
        }
        
        return RGB(r, g, b);
    }
    
    // Convert screen coordinates to complex plane
    std::complex<double> screen_to_complex(int x, int y) {
        double scale = 4.0 / zoom;
        double real = center_real + (x - width / 2.0) * scale / width;
        double imag = center_imag + (y - height / 2.0) * scale / height;
        return std::complex<double>(real, imag);
    }
    
    // Render fractal
    void render_fractal() {
        if (is_rendering.load()) return;
        is_rendering = true;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Multi-threaded rendering
        int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        int rows_per_thread = height / num_threads;
        
        for (int t = 0; t < num_threads; t++) {
            int start_row = t * rows_per_thread;
            int end_row = (t == num_threads - 1) ? height : start_row + rows_per_thread;
            
            threads.emplace_back([this, start_row, end_row]() {
                for (int y = start_row; y < end_row; y++) {
                    for (int x = 0; x < width; x++) {
                        std::complex<double> point = screen_to_complex(x, y);
                        
                        int iterations;
                        if (is_julia) {
                            iterations = julia_iterations(point);
                        } else {
                            iterations = mandelbrot_iterations(point);
                        }
                        
                        pixels[y * width + x] = get_color(iterations);
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::string title = "Interactive Fractal Explorer - ";
        title += is_julia ? "Julia Set" : "Mandelbrot Set";
        title += " - " + std::to_string(duration.count()) + "ms";
        title += " - Zoom: " + std::to_string((int)zoom) + "x";
        title += " - Iterations: " + std::to_string(max_iterations);
        
        SetWindowTextA(hwnd, title.c_str());
        InvalidateRect(hwnd, NULL, FALSE);
        
        is_rendering = false;
    }
    
    // Zoom to selected area
    void zoom_to_area(POINT start, POINT end) {
        POINT center_pixel = {(start.x + end.x) / 2, (start.y + end.y) / 2};
        std::complex<double> new_center = screen_to_complex(center_pixel.x, center_pixel.y);
        
        int selection_width = abs(end.x - start.x);
        int selection_height = abs(end.y - start.y);
        
        if (selection_width > 10 && selection_height > 10) {
            double zoom_factor = static_cast<double>(width) / selection_width;
            zoom *= zoom_factor;
            center_real = new_center.real();
            center_imag = new_center.imag();
            
            render_fractal();
        }
    }
    
    // Pan view
    void pan(POINT delta) {
        double scale = 4.0 / zoom;
        center_real -= delta.x * scale / width;
        center_imag -= delta.y * scale / height;
        render_fractal();
    }
    
    // Update Julia constant
    void update_julia_constant(POINT mouse_pos) {
        if (is_julia) {
            std::complex<double> new_c = screen_to_complex(mouse_pos.x, mouse_pos.y);
            double real_part = std::max(-2.0, std::min(2.0, new_c.real()));
            double imag_part = std::max(-2.0, std::min(2.0, new_c.imag()));
            julia_c = std::complex<double>(real_part, imag_part);
            render_fractal();
        }
    }
    
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        SimpleFractalViewer* viewer = reinterpret_cast<SimpleFractalViewer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        
        switch (uMsg) {
            case WM_CREATE:
                return 0;
                
            case WM_PAINT: {
                if (!viewer) break;
                
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                // Create bitmap and draw pixels
                BITMAPINFO bmi = {};
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = viewer->width;
                bmi.bmiHeader.biHeight = -viewer->height; // Top-down
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;
                
                SetDIBitsToDevice(hdc, 0, 0, viewer->width, viewer->height,
                                0, 0, 0, viewer->height, viewer->pixels.data(),
                                &bmi, DIB_RGB_COLORS);
                
                // Draw selection rectangle if selecting
                if (viewer->is_selecting) {
                    HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
                    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
                    
                    Rectangle(hdc, 
                            std::min(viewer->selection_start.x, viewer->selection_end.x),
                            std::min(viewer->selection_start.y, viewer->selection_end.y),
                            std::max(viewer->selection_start.x, viewer->selection_end.x),
                            std::max(viewer->selection_start.y, viewer->selection_end.y));
                    
                    SelectObject(hdc, oldPen);
                    DeleteObject(pen);
                }
                
                // Draw instructions
                HFONT font = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
                HFONT oldFont = (HFONT)SelectObject(hdc, font);
                
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkMode(hdc, TRANSPARENT);
                
                std::string instructions = "Controls:\n";
                instructions += "Left Click+Drag: Zoom\n";
                instructions += "Right Click+Drag: Pan\n";
                instructions += "M: Toggle Mandelbrot/Julia\n";
                instructions += "R: Reset view\n";
                instructions += "+/-: Iterations\n";
                instructions += "Mouse: Julia constant";
                
                RECT textRect = {10, 10, 300, 150};
                DrawTextA(hdc, instructions.c_str(), -1, &textRect, DT_LEFT | DT_TOP);
                
                // Show current mode
                std::string mode = viewer->is_julia ? "Julia Set Mode" : "Mandelbrot Set Mode";
                RECT modeRect = {10, viewer->height - 30, 300, viewer->height};
                DrawTextA(hdc, mode.c_str(), -1, &modeRect, DT_LEFT | DT_TOP);
                
                SelectObject(hdc, oldFont);
                DeleteObject(font);
                
                EndPaint(hwnd, &ps);
                return 0;
            }
            
            case WM_LBUTTONDOWN:
                if (viewer) {
                    viewer->is_selecting = true;
                    viewer->selection_start = {LOWORD(lParam), HIWORD(lParam)};
                    viewer->selection_end = viewer->selection_start;
                    SetCapture(hwnd);
                }
                return 0;
                
            case WM_LBUTTONUP:
                if (viewer && viewer->is_selecting) {
                    viewer->is_selecting = false;
                    viewer->selection_end = {LOWORD(lParam), HIWORD(lParam)};
                    viewer->zoom_to_area(viewer->selection_start, viewer->selection_end);
                    ReleaseCapture();
                }
                return 0;
                
            case WM_RBUTTONDOWN:
                if (viewer) {
                    viewer->is_dragging = true;
                    viewer->drag_start = {LOWORD(lParam), HIWORD(lParam)};
                    SetCapture(hwnd);
                }
                return 0;
                
            case WM_RBUTTONUP:
                if (viewer && viewer->is_dragging) {
                    viewer->is_dragging = false;
                    ReleaseCapture();
                }
                return 0;
                
            case WM_MOUSEMOVE: {
                if (!viewer) break;
                
                POINT mouse_pos = {LOWORD(lParam), HIWORD(lParam)};
                
                if (viewer->is_selecting) {
                    viewer->selection_end = mouse_pos;
                    InvalidateRect(hwnd, NULL, FALSE);
                } else if (viewer->is_dragging) {
                    POINT delta = {viewer->drag_start.x - mouse_pos.x, viewer->drag_start.y - mouse_pos.y};
                    viewer->pan(delta);
                    viewer->drag_start = mouse_pos;
                } else if (viewer->is_julia && !viewer->is_rendering.load()) {
                    viewer->update_julia_constant(mouse_pos);
                }
                return 0;
            }
            
            case WM_KEYDOWN:
                if (!viewer) break;
                
                switch (wParam) {
                    case VK_ESCAPE:
                        PostQuitMessage(0);
                        break;
                        
                    case 'M':
                        viewer->is_julia = !viewer->is_julia;
                        viewer->render_fractal();
                        break;
                        
                    case 'R':
                        viewer->zoom = 1.0;
                        viewer->center_real = viewer->is_julia ? 0.0 : -0.5;
                        viewer->center_imag = 0.0;
                        viewer->max_iterations = 100;
                        viewer->render_fractal();
                        break;
                        
                    case VK_OEM_PLUS:
                    case VK_ADD:
                        viewer->max_iterations = std::min(1000, viewer->max_iterations + 50);
                        viewer->render_fractal();
                        break;
                        
                    case VK_OEM_MINUS:
                    case VK_SUBTRACT:
                        viewer->max_iterations = std::max(50, viewer->max_iterations - 50);
                        viewer->render_fractal();
                        break;
                }
                return 0;
                
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    // Run the application
    void run() {
        // Register window class
        WNDCLASSA wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "FractalViewer";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        
        RegisterClassA(&wc);
        
        // Create window
        hwnd = CreateWindowExA(
            0,
            "FractalViewer",
            "Interactive Fractal Explorer",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, width + 16, height + 39,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );
        
        if (!hwnd) {
            std::cerr << "Failed to create window!" << std::endl;
            return;
        }
        
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        
        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);
        
        // Initial render
        render_fractal();
        
        // Message loop
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};

int main() {
    try {
        SimpleFractalViewer app(800, 600);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

#else
#error "This version is for Windows only. Use fractal_gui.cpp for cross-platform SFML version or fractal_sdl.cpp for SDL version."
#endif
