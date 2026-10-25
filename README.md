# GNSS Data Processing Engine

An independent C++17 GNSS software engine for processing raw GNSS measurements from u-blox ZED-F9P receivers. This project provides a clean, modular implementation for parsing UBX binary format, decoding multi-constellation observations, and preparing data for position calculation.

**Status:** Phase 1 Complete ✅ (Raw Data Parsing and Validation)

## Features

### ✅ Implemented (Phase 1)

- **UBX Binary Parser**
  - Frame synchronization with error recovery
  - Checksum validation
  - Robust handling of corrupted data
  - Support for concatenated messages

- **Message Decoders**
  - **UBX-RXM-RAWX:** Multi-GNSS raw measurements (pseudorange, carrier phase, Doppler)
  - **UBX-RXM-SFRBX:** Navigation message subframes
  - **UBX-NAV-PVT:** Receiver position/velocity/time solutions (optional)

- **Multi-GNSS Support**
  - GPS (L1C/A, L2C)
  - GLONASS (G1, G2 with FDMA channels)
  - Galileo (E1, E5a)
  - BeiDou (B1I, B2I)
  - QZSS (L1C/A, L5)
  - SBAS (L1C/A)

- **Data Models**
  - Clean C++ observation structures
  - Validity flags for each measurement type
  - Signal quality indicators
  - Constellation and signal identification

- **Signal Registry**
  - Frequency and wavelength lookup
  - GLONASS FDMA support
  - Signal naming and classification

- **Command-Line Tool: `gnss_inspect`**
  - File statistics and parser diagnostics
  - Epoch listing and detailed views
  - Satellite/signal summaries
  - CSV export for observations, navigation messages, and PVT solutions

- **Command-Line Tool: `gnss_decode`** ⭐ NEW
  - Human-readable output of all decoded data
  - Print observations with full details to console
  - Perfect for studying GNSS measurements
  - Filter by epoch or message type

- **Unit Tests**
  - Parser validation
  - Decoder tests
  - Signal registry tests
  - Error handling tests

- **Validation**
  - Compared against RTKLIB 2.4.2
  - Exact match with RINEX output
  - Validated on 50 minutes of real ZED-F9P data

### 🚧 Not Implemented (Future Phases)

- Broadcast ephemeris decoding
- Satellite position/clock calculations
- Single Point Positioning (SPP)
- Atmospheric corrections (ionosphere, troposphere)
- Kalman filtering
- Relative positioning (RTK)
- Ambiguity resolution
- RTCM support

## Build Requirements

- **C++ Compiler:** C++17 support required (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake:** Version 3.14 or later
- **Operating Systems:** Linux, Windows 11

**No external dependencies required** - the project uses only standard C++17 library features.

## Building the Project

### Linux

```bash
# Clone or navigate to the project directory
cd gnss-engine

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j4

# Run tests
ctest --output-on-failure
```

### Windows

```cmd
# Create build directory
mkdir build
cd build

# Configure with CMake (Visual Studio)
cmake -G "Visual Studio 16 2019" ..

# Build
cmake --build . --config Release

# Run tests
ctest -C Release --output-on-failure
```

### Build Options

```bash
cmake -DBUILD_TESTS=ON -DBUILD_TOOLS=ON ..
```

- `BUILD_TESTS`: Build unit tests (default: ON)
- `BUILD_TOOLS`: Build command-line tools (default: ON)

## Usage

### Command-Line Tool

```bash
# Basic usage - parse and report statistics
./gnss_inspect data/rover.ubx

# List all epochs
./gnss_inspect -e data/rover.ubx

# Show detailed measurements for epoch 0
./gnss_inspect -d 0 data/rover.ubx

# Print satellite/signal summary
./gnss_inspect -s data/rover.ubx

# Export observations to CSV
./gnss_inspect --csv-obs observations.csv data/rover.ubx

# Export navigation messages to CSV
./gnss_inspect --csv-nav navigation.csv data/rover.ubx

# Export PVT solutions to CSV (if present)
./gnss_inspect --csv-pvt pvt.csv data/rover.ubx

# Combine options
./gnss_inspect -s --csv-obs obs.csv data/rover.ubx
```

### Example Output

```
GNSS Data Inspector
===================

Input file: data/rover.ubx
File size: 9332064 bytes (8.90 MB)

Parsing file...
Parsing complete.

=== Parser Statistics ===
Total bytes read:       9332064
Messages parsed:        63884
Checksum failures:      0
Sync errors:            0
Malformed messages:     0

Message type counts:
  RXM-RAWX:             2977
  RXM-SFRBX:            60838
  NAV-PVT:              0
  Other:                69

=== Data Summary ===
Time span:
  Start: GPS Week 2188, TOW 207960.005 s
  End:   GPS Week 2188, TOW 210936.007 s
  Duration: 2976.002 s

Epochs:
  Total epochs: 2977
  Average rate: 1.00 Hz

Observations:
  Total measurements: 194610
  Valid pseudorange:  194610
  Valid carrier phase: 155232
  Valid Doppler:      194610

Observations by constellation:
  GPS       : 47973 obs, 10 satellites
  Galileo   : 38776 obs, 8 satellites
  BeiDou    : 42209 obs, 10 satellites
  GLONASS   : 38492 obs, 9 satellites
  QZSS      : 18229 obs, 4 satellites
  SBAS      : 8931 obs, 3 satellites
```

### CSV Export Format

#### Observations CSV

Columns:
- `GPS_Week`: GPS week number
- `TOW_s`: Time of week (seconds)
- `Constellation`: GNSS constellation name
- `SatID`: Satellite identifier
- `SignalID`: Signal identifier
- `FreqID`: Frequency channel (GLONASS only)
- `Pseudorange_m`: Pseudorange in metres
- `PR_Valid`: Pseudorange validity (0/1)
- `Carrier_Phase_cy`: Carrier phase in cycles
- `CP_Valid`: Carrier phase validity (0/1)
- `Doppler_Hz`: Doppler frequency in Hz
- `DO_Valid`: Doppler validity (0/1)
- `CN0_dBHz`: Carrier-to-noise density in dB-Hz
- `LockTime_raw`: Carrier lock time (raw units)
- `PR_StdDev`, `CP_StdDev`, `DO_StdDev`: Standard deviation indicators
- `HalfCycleValid`, `HalfCycleSubtracted`: Half-cycle status flags

## Using as a Library

### Example Code

```cpp
#include "gnss/ubx_parser.hpp"
#include <iostream>

int main() {
    gnss::UbxParser parser;
    
    // Set callback for observations
    parser.set_observation_callback([](const gnss::ObservationEpoch& epoch) {
        std::cout << "Epoch: GPS Week " << epoch.gps_week
                  << ", TOW " << epoch.time_of_week_s << " s\n";
        std::cout << "Observations: " << epoch.observations.size() << "\n";
        
        for (const auto& obs : epoch.observations) {
            if (obs.pseudorange_valid) {
                std::cout << "  " << gnss::constellation_to_string(obs.constellation)
                          << " " << (int)obs.satellite_id
                          << " PR: " << obs.pseudorange_m << " m\n";
            }
        }
    });
    
    // Parse file
    if (parser.parse_file("data/rover.ubx")) {
        const auto& stats = parser.get_statistics();
        std::cout << "Parsed " << stats.messages_parsed << " messages\n";
    }
    
    return 0;
}
```

### Link Against the Library

```cmake
find_package(gnss_engine REQUIRED)
target_link_libraries(your_app gnss_engine)
```

## Project Structure

```
gnss-engine/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── docs/
│   ├── architecture.md         # Software architecture
│   ├── ubx-message-support.md  # UBX message documentation
│   ├── data-model.md           # Data structures
│   └── validation-report.md    # Validation results
├── include/gnss/
│   ├── ubx_parser.hpp          # UBX parser interface
│   ├── observation.hpp         # Observation data models
│   ├── signal_registry.hpp     # Signal frequency/wavelength lookup
│   └── diagnostics.hpp         # Analysis and export utilities
├── src/
│   ├── ubx/
│   │   ├── ubx_parser.cpp      # Frame sync and message extraction
│   │   ├── rawx_decoder.cpp    # RAWX message decoder
│   │   ├── sfrbx_decoder.cpp   # SFRBX message decoder
│   │   └── nav_pvt_decoder.cpp # NAV-PVT message decoder
│   ├── gnss/
│   │   ├── signal_registry.cpp # Signal database
│   │   ├── observation.cpp     # Data model implementations
│   │   └── diagnostics.cpp     # Analysis tools
│   └── tools/
│       └── gnss_inspect.cpp    # Command-line tool
├── tests/
│   ├── test_ubx_parser.cpp     # Parser unit tests
│   ├── test_rawx.cpp           # RAWX decoder tests
│   ├── test_sfrbx.cpp          # SFRBX decoder tests
│   └── test_signal_registry.cpp # Signal registry tests
├── data/
│   └── rover.ubx               # Sample ZED-F9P recording
└── RTKLIB/                     # Reference implementation (read-only)
```

## Data Models

### ObservationEpoch

Represents all observations at a single time instant.

```cpp
struct ObservationEpoch {
    int gps_week;
    double time_of_week_s;
    uint8_t leap_seconds;
    bool leap_seconds_valid;
    std::vector<GnssObservation> observations;
};
```

### GnssObservation

Single satellite signal measurement.

```cpp
struct GnssObservation {
    GnssConstellation constellation;
    uint8_t satellite_id;
    uint8_t signal_id;
    int8_t frequency_id;  // GLONASS only
    
    // Measurements
    double pseudorange_m;
    double carrier_phase_cycles;
    double doppler_hz;
    double cn0_db_hz;
    
    // Quality indicators
    uint16_t lock_time_raw;
    uint8_t pr_stdev, cp_stdev, do_stdev;
    
    // Validity flags
    bool pseudorange_valid;
    bool carrier_phase_valid;
    bool doppler_valid;
    bool half_cycle_valid;
    bool half_cycle_subtracted;
};
```

## Signal Identification

The signal registry provides frequency and wavelength information for each supported signal:

```cpp
auto info = gnss::SignalRegistry::get_signal_info(
    gnss::GnssConstellation::GPS, 0);
// Returns: GPS L1C/A, 1575.42 MHz, wavelength ~0.19 m

auto freq = gnss::SignalRegistry::get_carrier_frequency(
    gnss::GnssConstellation::GLONASS, 0, channel_number);
// Returns frequency accounting for GLONASS FDMA channel
```

## Unit Conventions

- **Time:** GPS week number + time of week in seconds
- **Distance:** Metres (pseudorange, height)
- **Carrier Phase:** Cycles (not converted to metres)
- **Doppler:** Hertz
- **Signal Strength:** dB-Hz
- **Angles:** Degrees (latitude, longitude, heading)
- **Velocity:** Metres per second

## Validation

The implementation has been validated against RTKLIB 2.4.2:

- ✅ **Pseudorange:** Exact match
- ✅ **Carrier Phase:** Exact match  
- ✅ **Doppler:** Exact match
- ✅ **Signal Strength:** Exact match
- ✅ **Epoch Count:** 2,977 epochs validated
- ✅ **Observation Count:** 194,610 measurements validated
- ✅ **Zero parsing errors** on 8.9 MB of real data

See [docs/validation-report.md](docs/validation-report.md) for complete validation results.

## Known Limitations

1. **Navigation Data:** SFRBX messages are captured but ephemeris decoding is not yet implemented
2. **Position Calculation:** No SPP or RTK implementation in Phase 1
3. **RTCM:** No support for differential corrections
4. **Signal Support:** Some less-common signals may not be in the registry

## Licensing and Attribution

### RTKLIB Reference

This project uses [RTKLIB](http://www.rtklib.com/) version 2.4.2 patch 13 as a reference implementation for validation purposes.

**Copyright:** Copyright (C) 2007-2018 by T. Takasu, All rights reserved.

**License:** BSD 2-Clause License

The RTKLIB source code in the `RTKLIB/` directory retains its original copyright and license. Our custom implementation is independently written and does not incorporate RTKLIB source code, but uses RTKLIB for:
- Understanding UBX protocol structure
- Validating decoded measurements
- RINEX comparison

As specified in the project requirements:
- RTKLIB informed the implementation through its protocol handling
- No RTKLIB code was copied into the custom engine
- Validation was performed by comparing outputs

### This Project

The custom GNSS engine source code (all files outside `RTKLIB/`) is original work created for this project.

## Development Principles

The implementation follows these principles:

1. **Modern C++17:** Use standard library features, avoid C-style arrays
2. **Explicit Units:** Document units for all measurements
3. **Validity Flags:** Never silently discard or fabricate measurements
4. **SI Units:** Use metres, seconds, hertz internally
5. **No Magic Numbers:** Named constants for all protocol values
6. **Error Handling:** Graceful recovery from corrupted data
7. **Separation of Concerns:** Binary parsing ≠ data models ≠ positioning
8. **Testability:** Unit tests for all decoders
9. **Documentation:** Explain GNSS concepts and implementation choices

## Future Development Roadmap

### Phase 2: Ephemeris and Satellite Positions
- Decode GPS/Galileo/BeiDou broadcast ephemeris
- Calculate satellite positions using Keplerian parameters
- Compute satellite clock corrections
- Time system conversions (GPS, GLONASS, Galileo, BeiDou)

### Phase 3: Single Point Positioning
- Implement least-squares position estimation
- Tropospheric delay models (Saastamoinen, UNB3m)
- Ionospheric corrections (Klobuchar, NeQuick)
- DOP calculations and quality metrics

### Phase 4: Kalman Filtering
- Extended Kalman Filter for kinematic positioning
- Velocity estimation from Doppler
- Outlier detection and exclusion

### Phase 5: Relative Positioning (RTK)
- Base station data handling (RTCM)
- Double-difference formation
- Float ambiguity resolution
- Integer ambiguity resolution (LAMBDA)

## Contributing

Contributions are welcome! Areas of interest:
- Additional signal support (L5, E5b, B1C, B2a)
- More comprehensive unit tests
- Performance optimization
- Windows build testing
- Documentation improvements

## References

1. **u-blox ZED-F9P Interface Description:** UBX-18010854
2. **RTKLIB Manual:** T. Takasu, RTKLIB ver. 2.4.2 Manual, 2013
3. **GPS Interface Specification:** IS-GPS-200 Revision L
4. **GLONASS ICD:** Edition 5.1, 2008
5. **Galileo ICD:** European GNSS (Galileo) Open Service Signal In Space Interface Control Document
6. **BeiDou ICD:** BeiDou Navigation Satellite System Signal In Space Interface Control Document

## Contact and Support

For questions about this implementation:
- Review the [validation report](docs/validation-report.md)
- Check the [architecture documentation](docs/architecture.md)
- Examine the unit tests for usage examples

---

**Project Status:** Phase 1 Complete ✅  
**Last Updated:** December 2026  
**C++ Standard:** C++17  
**Build System:** CMake 3.14+
