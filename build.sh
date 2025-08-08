#!/bin/bash

echo "Building PixLegends..."

# Create build directory if it doesn't exist
mkdir -p build

# Change to build directory
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build the project
echo "Building project..."
make -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Executable created in: $(pwd)/PixLegends"
    echo ""
    echo "To run the game, execute: ./PixLegends"
else
    echo "Build failed!"
    exit 1
fi

cd ..
