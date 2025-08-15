# Makefile untuk kompilasi program Mandelbrot
# Requires CUDA Toolkit untuk versi GPU

CC = gcc
NVCC = nvcc
CFLAGS = -O2 -fopenmp -Wall
NVCCFLAGS = -O2 -Xcompiler -fopenmp

# Target default
all: serial parallel

# Versi serial (minimal)
serial: serial.c
	$(CC) -O2 -o mandelbrot_serial serial.c

# Versi paralel dengan OpenMP
parallel: parallel.c
	$(CC) $(CFLAGS) -o mandelbrot_parallel parallel.c

# Versi GPU dengan CUDA (optional - requires CUDA SDK)
gpu: gpu.c
	@echo "Attempting to compile CUDA version..."
	@if command -v nvcc >/dev/null 2>&1; then \
		$(NVCC) $(NVCCFLAGS) -o mandelbrot_gpu gpu.c; \
		echo "✅ GPU version compiled successfully!"; \
	else \
		echo "❌ CUDA compiler (nvcc) not found. Please install CUDA SDK."; \
		echo "You can download it from: https://developer.nvidia.com/cuda-downloads"; \
		exit 1; \
	fi

# Test semua versi yang tersedia
test: all
	@echo "=== Testing Serial Version ==="
	./mandelbrot_serial
	@echo "=== Testing Parallel Version ==="
	./mandelbrot_parallel
	@if [ -f mandelbrot_gpu ]; then \
		echo "=== Testing GPU Version ==="; \
		./mandelbrot_gpu; \
	fi

# Bersihkan file hasil kompilasi
clean:
	rm -f mandelbrot_serial mandelbrot_parallel mandelbrot_gpu
	rm -f *.bmp

# Install dependencies (Ubuntu/Debian)
install-deps:
	@echo "Installing dependencies..."
	sudo apt update
	sudo apt install gcc libomp-dev

# Install CUDA (Ubuntu/Debian)
install-cuda:
	@echo "This will guide you to install CUDA..."
	@echo "Please visit: https://developer.nvidia.com/cuda-downloads"
	@echo "And follow instructions for your system."

help:
	@echo "Available targets:"
	@echo "  all       - Compile serial and parallel versions"
	@echo "  serial    - Compile serial version only"
	@echo "  parallel  - Compile parallel version only" 
	@echo "  gpu       - Compile GPU version (requires CUDA)"
	@echo "  test      - Run all compiled versions"
	@echo "  clean     - Remove compiled files and images"
	@echo "  help      - Show this help"

.PHONY: all serial parallel gpu test clean install-deps install-cuda help
