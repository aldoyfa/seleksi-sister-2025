#!/bin/bash
echo "Compiling NTT-based big integer multiplication program..."
gcc program.c -O2 -std=c11 -o program
if [ $? -eq 0 ]; then
    echo "Compilation successful! program executable created."
else
    echo "Compilation failed with error code $?"
fi
