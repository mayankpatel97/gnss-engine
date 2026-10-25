# GNSS Engine Usage Guide

## Tools Overview

The GNSS engine provides two command-line tools:

### 1. `gnss_decode` - Human-Readable Decoder (NEW!)

**Purpose:** Print all decoded GNSS data to console for study and analysis.

**Best for:**
- Learning about GNSS measurements
- Understanding raw data structure
- Debugging receiver output
- Studying signal characteristics

### 2. `gnss_inspect` - Statistics and CSV Export

**Purpose:** Quick file analysis and bulk data export.

**Best for:**
- File statistics
- CSV export for further analysis
- Batch processing
- Validation checks

---

## Using `gnss_decode` (Study Mode)

### Basic Usage

```bash
# Decode and print all observations
./build/gnss_decode data/rover.ubx

# Show first 5 epochs only
./build/gnss_decode data/rover.ubx --max-epochs 5

# Show specific epoch (e.g., epoch 10)
./build/gnss_decode data/rover.ubx --epoch 10

# Show only observations (skip navigation messages)
./build/gnss_decode data/rover.ubx --obs-only

# Show only navigation messages
./build/gnss_decode data/rover.ubx --nav-only
```

### Output Format

For each observation epoch, you'll see:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ EPOCH #    0                                                               │
├─────────────────────────────────────────────────────────────────────────────┤
│ GPS Week:          2188                                                   │
│ Time of Week:      207960.005 seconds                                     │
│ Leap Seconds:    18 (GPS-UTC offset)                                     │
│ Observations:     58 measurements                                         │
└─────────────────────────────────────────────────────────────────────────────┘

  ┌─ GPS (15 observations) ──────────────────────────────────────────────────┐
  │
  │ Satellite:  GPS SV  4  Signal: GPS L1C/A
  │   Frequency:    1575.420 MHz
  │   Wavelength:   0.1903 metres
  │
  │   Pseudorange:    21743857.636 metres [VALID]  (σ=3)
  │   Carrier Phase: 114264745.094 cycles  [VALID]  (σ=1)
  │                   21743858.015 metres  (converted)
  │   Doppler:           -1714.171 Hz      [VALID]  (σ=6)
  │
  │   Signal Quality:
  │     C/N0:           45.0 dB-Hz
  │     Lock Time:      64500 ms (raw)
```

### Understanding the Output

**Time Information:**
- **GPS Week**: Current GPS week number (week 0 = Jan 6, 1980)
- **Time of Week**: Seconds since start of GPS week (Sunday 00:00:00)
- **Leap Seconds**: Offset between GPS time and UTC

**For Each Observation:**
- **Satellite**: Constellation and satellite number
- **Signal**: Signal type and name
- **Frequency**: Carrier frequency in MHz
- **Wavelength**: Signal wavelength in metres
- **Pseudorange**: Distance to satellite (includes clock offsets)
- **Carrier Phase**: Phase measurement in cycles (and converted to metres)
- **Doppler**: Frequency shift in Hz (related to velocity)
- **C/N0**: Signal strength (carrier-to-noise density ratio)
- **Lock Time**: How long the receiver has been tracking this signal
- **σ (sigma)**: Standard deviation indicator (lower is better)

**Validity Flags:**
- `[VALID]` - Measurement is reliable
- `NOT AVAILABLE` - Measurement not available or invalid

**Special Markers:**
- `[Half-cycle subtracted]` - Phase ambiguity resolved
- `[WARNING: Half-cycle ambiguity]` - Phase measurement uncertain by half wavelength

---

## Using `gnss_inspect` (Analysis Mode)

### Basic Analysis

```bash
# Quick file statistics
./build/gnss_inspect data/rover.ubx

# Show all epochs
./build/gnss_inspect -e data/rover.ubx

# Detailed view of epoch 0
./build/gnss_inspect -d 0 data/rover.ubx

# Satellite/signal summary
./build/gnss_inspect -s data/rover.ubx
```

### CSV Export

```bash
# Export observations to CSV
./build/gnss_inspect --csv-obs observations.csv data/rover.ubx

# Export navigation messages
./build/gnss_inspect --csv-nav navigation.csv data/rover.ubx

# Export PVT solutions (if present)
./build/gnss_inspect --csv-pvt pvt.csv data/rover.ubx

# Export everything
./build/gnss_inspect \
    --csv-obs observations.csv \
    --csv-nav navigation.csv \
    data/rover.ubx
```

### Combining Options

```bash
# Statistics + satellite summary + CSV export
./build/gnss_inspect -s --csv-obs data.csv data/rover.ubx
```

---

## Real-Time UART Usage (Future)

The parser is **already designed for streaming data**. To use with UART:

### Linux Example

```cpp
#include "gnss/ubx_parser.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

int main() {
    // Open serial port
    int uart = open("/dev/ttyUSB0", O_RDONLY | O_NOCTTY);
    
    // Configure baud rate (115200 for ZED-F9P)
    struct termios options;
    tcgetattr(uart, &options);
    cfsetispeed(&options, B115200);
    options.c_cflag |= (CLOCAL | CREAD);
    tcsetattr(uart, TCSANOW, &options);
    
    // Setup parser
    gnss::UbxParser parser;
    parser.set_observation_callback([](const gnss::ObservationEpoch& epoch) {
        std::cout << "Real-time epoch: " << epoch.observations.size() << " obs\n";
    });
    
    // Read and parse in real-time
    uint8_t byte;
    while (read(uart, &byte, 1) > 0) {
        parser.parse_buffer(&byte, 1);  // Process one byte at a time
    }
    
    close(uart);
    return 0;
}
```

### Key Points for UART

1. **Byte-by-byte processing**: The parser handles streaming perfectly
2. **No buffering required**: Feed bytes directly as they arrive
3. **Automatic synchronization**: Parser finds UBX frames automatically
4. **Error recovery**: Corrupted data won't crash the parser
5. **Low latency**: Callbacks fire immediately when messages complete

### Baud Rate Recommendations

- **u-blox ZED-F9P**: 115200 or 230400 baud (configurable)
- **Message rate**: 1-20 Hz typical for raw measurements
- **Data volume**: ~3-5 KB/s at 1 Hz with multi-constellation

---

## Understanding GNSS Measurements

### Pseudorange

**What it is:** Travel time of signal × speed of light

**Value range:** Typically 20,000 km to 26,000 km from Earth surface

**Includes:**
- Geometric distance to satellite
- Receiver clock offset
- Satellite clock offset
- Atmospheric delays
- Measurement noise

**Why useful:** Primary measurement for position calculation

### Carrier Phase

**What it is:** Number of signal wavelengths between satellite and receiver

**Value range:** Millions to billions of cycles

**Accuracy:** ~1-2 millimeters (vs ~30 cm for pseudorange)

**Challenges:**
- Integer ambiguity (unknown number of full wavelengths)
- Half-cycle ambiguity (180° phase uncertainty)
- Cycle slips (loss of lock)

**Why useful:** High-precision positioning (RTK)

### Doppler

**What it is:** Frequency shift due to relative motion

**Value range:** Typically ±5 kHz

**Sign convention:**
- Negative: Satellite moving away
- Positive: Satellite approaching

**Why useful:**
- Velocity estimation
- Helps resolve cycle slips
- Improves convergence

### Signal Quality (C/N0)

**What it is:** Carrier-to-noise density ratio

**Value range:** 
- 30-35 dB-Hz: Weak (indoors, obstructions)
- 35-40 dB-Hz: Fair
- 40-45 dB-Hz: Good (typical outdoor)
- 45+ dB-Hz: Excellent (clear sky)

**Why useful:**
- Weight measurements in position solution
- Detect multipath
- Assess tracking quality

---

## Typical Workflow

### 1. First Look at Data

```bash
# Get overview
./build/gnss_inspect data/recording.ubx
```

Check:
- How many epochs?
- Which constellations?
- Signal quality?
- Any errors?

### 2. Study Detailed Measurements

```bash
# Look at first few epochs
./build/gnss_decode data/recording.ubx --max-epochs 5
```

Understand:
- What satellites are visible?
- What signals are tracked?
- Are measurements valid?
- Any anomalies?

### 3. Export for Analysis

```bash
# Export to CSV
./build/gnss_inspect --csv-obs measurements.csv data/recording.ubx
```

Then:
- Import into Python/MATLAB/Excel
- Plot pseudorange vs time
- Analyze Doppler
- Check carrier phase continuity

### 4. Specific Investigation

```bash
# Look at specific epoch
./build/gnss_decode data/recording.ubx --epoch 42

# Or range of epochs
./build/gnss_decode data/recording.ubx --max-epochs 100
```

---

## Common Questions

### Q: Why are some carrier phases invalid?

**A:** Carrier phase requires continuous tracking. Invalid when:
- Signal too weak
- Cycle slip occurred
- Just started tracking
- Obstructions/multipath

### Q: What does "Half-cycle subtracted" mean?

**A:** The receiver resolved a 180° phase ambiguity and adjusted the measurement. This is normal and correct.

### Q: Why do pseudoranges look so large?

**A:** They include clock offsets! The actual geometric distance is:
```
Geometric range = Pseudorange - (Receiver clock offset) + (Satellite clock offset)
```
Clock offsets can be hundreds of kilometers equivalent!

### Q: What's the difference between cycles and metres for phase?

**A:** 
- **Cycles**: Raw measurement from receiver
- **Metres**: Cycles × wavelength
- Example: 114,264,745 cycles × 0.1903 m = 21,743,858 metres

### Q: Can I use this for position calculation?

**A:** Phase 1 provides the measurements. Phase 2 will add:
- Ephemeris decoding (satellite positions)
- Clock corrections
- Position estimation (SPP/RTK)

---

## Tips for Learning

1. **Start Small**: Look at just 1-2 epochs first
2. **Compare Constellations**: Notice GPS vs Galileo vs BeiDou
3. **Watch Signal Strength**: C/N0 tells you a lot about tracking
4. **Track One Satellite**: Follow one satellite across multiple epochs
5. **Study Doppler Patterns**: Shows satellite motion relative to you

---

## Next Steps

Once you understand the measurements:

1. **Study Ephemeris**: Learn how satellite positions are broadcast
2. **Calculate Satellite Positions**: Use Keplerian elements
3. **Correct Measurements**: Apply clock and atmospheric corrections
4. **Solve for Position**: Implement least-squares or Kalman filter

The measurements are ready - now you can build the positioning algorithms!

---

**Have questions?** Check the [Architecture Documentation](docs/architecture.md) or [Validation Report](docs/validation-report.md).
