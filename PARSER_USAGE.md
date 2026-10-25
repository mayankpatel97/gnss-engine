# GNSS Parser Usage

## Quick Start

The `gnss_parser` tool extracts measurements from UBX files and outputs them in table format.

### Basic Usage

```bash
./build/gnss_parser <input.ubx> [output.txt]
```

### Examples

```bash
# Parse UBX file, output to console and measurements.txt (default)
./build/gnss_parser data/rover.ubx

# Parse UBX file, specify output filename
./build/gnss_parser data/rover.ubx my_output.txt
```

## Output Format

### Table Columns

| Column | Description | Units | Notes |
|--------|-------------|-------|-------|
| **Const** | Constellation | - | GPS, Galileo, BeiDou, GLONASS, QZSS, SBAS |
| **SV** | Satellite ID | - | PRN or slot number |
| **SigID** | Signal ID | - | 0=L1/E1/B1, 3=L2, etc. |
| **Pseudorange(m)** | Raw pseudorange | metres | Distance + clock offsets |
| **V** | Valid flag | Y/N | Y=valid, N=invalid |
| **CarrierPhase(cy)** | Carrier phase | cycles | High-precision measurement |
| **V** | Valid flag | Y/N | Y=valid, N=invalid |
| **Doppler(Hz)** | Frequency shift | Hz | Related to velocity |
| **V** | Valid flag | Y/N | Y=valid, N=invalid |
| **CN0(dBHz)** | Signal strength | dB-Hz | Higher is better |
| **Signal** | Signal name | - | Human-readable signal type |

### Example Output

```
═══════════════════════════════════════════════════════════════════════════
 EPOCH #    0  │  GPS Week: 2188  │  TOW:   207960.005 s  │  Observations:  58
═══════════════════════════════════════════════════════════════════════════
Const     SV   SigID   Pseudorange(m)   V  CarrierPhase(cy)    V  Doppler(Hz) V  CN0(dBHz)Signal      
---------------------------------------------------------------------------------------------------------
GPS       4    0       21743857.636     Y  114264745.094       Y  -1714.171   Y  45.0     GPS L1C/A   
GPS       8    0       23243097.748     Y  122143307.143       Y  986.858     Y  41.0     GPS L1C/A   
Galileo   2    0       28260797.201     Y  148511490.763       Y  806.641     Y  39.0     GAL E1 C    
BeiDou    6    0       38191215.011     Y  198871674.959       Y  -1305.244   Y  38.0     BDS B1I D1  
GLONASS   2    0       20630913.067     Y  110090507.017       Y  -876.922    Y  42.0     GLO G1 C/A  
...
```

## Understanding the Data

### Pseudorange

- **What**: Travel time × speed of light
- **Range**: 20,000 km to 26,000 km (typical)
- **Includes**: 
  - Geometric distance to satellite
  - Receiver clock offset (~microseconds = ~300 km!)
  - Satellite clock offset
  - Atmospheric delays
- **Use**: Primary measurement for position calculation

### Carrier Phase

- **What**: Number of signal wavelengths
- **Range**: Millions to billions of cycles
- **Precision**: ~1-2 mm (vs ~30 cm for pseudorange)
- **Challenge**: Integer ambiguity (unknown number of full cycles)
- **Use**: High-precision positioning (RTK)

### Doppler

- **What**: Frequency shift due to motion
- **Range**: Typically ±5 kHz
- **Sign**: 
  - Negative = satellite moving away
  - Positive = satellite approaching
- **Use**: Velocity estimation

### Signal Strength (CN0)

- **Range**:
  - <30 dB-Hz: Very weak
  - 30-35 dB-Hz: Weak signal
  - 35-40 dB-Hz: Fair
  - 40-45 dB-Hz: Good
  - >45 dB-Hz: Excellent
- **Use**: Weight measurements, detect multipath

## Summary Statistics

At the end, you get:

```
═══════════════════════════════════════════════════════════════
 SUMMARY
═══════════════════════════════════════════════════════════════
 Total Epochs:           2977
 Total Observations:     194610
 Valid Pseudorange:      194610 (100%)
 Valid Carrier Phase:    155232 (79.77%)
 Valid Doppler:          194610 (100%)
═══════════════════════════════════════════════════════════════

Parser Statistics:
------------------
Messages parsed:      63884
Checksum failures:    0
Sync errors:          0

✓ Parsing completed successfully!
```

## What You Can Do With This Data

### 1. Current Phase (Parsing) ✅

- Extract measurements from UBX files
- View all satellites and signals
- Check measurement quality
- Validate data integrity

### 2. Next Phase (Position Calculation) 🚧

You will use these measurements to:

1. **Decode Ephemeris** (from navigation messages)
   - Get satellite orbital parameters
   - Calculate satellite positions at measurement time

2. **Apply Corrections**
   - Satellite clock corrections
   - Atmospheric delays (ionosphere, troposphere)
   - Relativistic effects

3. **Calculate Position**
   - Use pseudoranges in least-squares solver
   - Solve for receiver position (X, Y, Z)
   - Convert to latitude, longitude, height

4. **Generate NMEA**
   - Format position as NMEA sentences
   - Output: `$GPGGA,...`, `$GPRMC,...`, etc.

## File Locations

```
Input:  data/rover.ubx           # Raw UBX recording
Output: measurements.txt         # Parsed measurements table
```

## Processing Large Files

For large files, you can:

```bash
# View first few epochs
head -500 measurements.txt

# View last few epochs
tail -500 measurements.txt

# Count epochs
grep -c "EPOCH #" measurements.txt

# Extract just GPS measurements
grep "GPS" measurements.txt

# Find specific satellite
grep "GPS.*4" measurements.txt
```

## Using With Your Own Data

### From File

```bash
# Your recorded UBX file
./build/gnss_parser /path/to/your/recording.ubx output.txt
```

### From UART (Future)

The parser is ready for real-time UART streaming:

```cpp
// Read from serial port byte-by-byte
uint8_t byte;
while (read(uart_fd, &byte, 1) > 0) {
    parser.parse_buffer(&byte, 1);
}
```

See `UART_STREAMING.md` for complete UART examples.

## Troubleshooting

### No Output

- Check input file exists and is readable
- Verify file is UBX format (not NMEA)
- Check file permissions

### Checksum Failures

- Normal for noisy data
- If >10%, check data quality
- Verify baud rate matches (for UART)

### No Carrier Phase

- Normal for weak signals
- Requires continuous tracking
- Check CN0 values (should be >35 dB-Hz)

## Next Steps

Once you have parsed measurements:

1. Study the data format
2. Understand measurement types
3. Check data quality (CN0, validity)
4. Ready for Phase 2: Position calculation

---

**Tools Available:**

- `gnss_parser` - Table output (this tool) ← **Use this for parsing**
- `gnss_inspect` - Statistics and CSV export
- `gnss_decode` - Detailed human-readable output

**Recommended:** Use `gnss_parser` for clean table output, both to console and file.
