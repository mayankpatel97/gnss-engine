# Build and Test Instructions

## Quick Start

```bash
# Navigate to project
cd gnss-engine

# Create build directory
mkdir -p build && cd build

# Configure
cmake ..

# Build
make -j4

# Run tests
ctest --output-on-failure

# Test the tool
cd ..
./build/gnss_inspect data/rover.ubx
```

## Build Options

### Enable/Disable Components

```bash
cmake -DBUILD_TESTS=ON -DBUILD_TOOLS=ON ..
```

### Build Types

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (optimized)
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Running Tests

```bash
# All tests
ctest

# Verbose output
ctest --output-on-failure

# Specific test
ctest -R test_ubx_parser

# List available tests
ctest -N
```

## Tool Usage

```bash
# Basic analysis
./build/gnss_inspect data/rover.ubx

# Export to CSV
./build/gnss_inspect --csv-obs observations.csv data/rover.ubx

# Detailed epoch view
./build/gnss_inspect -d 0 data/rover.ubx

# Satellite summary
./build/gnss_inspect -s data/rover.ubx

# Help
./build/gnss_inspect --help
```

## Clean Build

```bash
rm -rf build
mkdir build && cd build
cmake ..
make -j4
```

## Installation

```bash
cd build
sudo make install
```

Default installation paths:
- Binaries: `/usr/local/bin/`
- Libraries: `/usr/local/lib/`
- Headers: `/usr/local/include/gnss/`

## Windows Build (Visual Studio)

```cmd
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
ctest -C Release
```

## Dependencies

**None!** The project uses only C++17 standard library.

## Compiler Requirements

- GCC 7.0+ or Clang 5.0+ or MSVC 2017+
- C++17 support required
- CMake 3.14 or later

## Troubleshooting

### "C++ compiler doesn't support C++17"

Update your compiler or specify a newer one:

```bash
cmake -DCMAKE_CXX_COMPILER=g++-9 ..
```

### Test failures

Run with verbose output:

```bash
ctest --output-on-failure --verbose
```

### Build warnings

The project builds cleanly with `-Wall -Wextra`. Any warnings should be reported as issues.
