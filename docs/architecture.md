# GNSS Engine Architecture

## Design Overview

The GNSS engine follows a layered architecture that separates concerns:

```
┌─────────────────────────────────────┐
│     Application Layer               │
│  (gnss_inspect, user applications)  │
└─────────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────┐
│     Analysis & Export Layer         │
│  (DiagnosticAnalyzer, CsvExporter)  │
└─────────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────┐
│     Data Model Layer                │
│  (ObservationEpoch, GnssObservation │
│   NavigationMessage, ReceiverPVT)   │
└─────────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────┐
│     Decoder Layer                   │
│  (RawxDecoder, SfrbxDecoder,        │
│   NavPvtDecoder, SignalRegistry)    │
└─────────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────┐
│     Protocol Layer                  │
│  (UbxParser - frame sync, checksum) │
└─────────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────┐
│     Input Layer                     │
│  (Binary UBX files, streams)        │
└─────────────────────────────────────┘
```

## Layer Responsibilities

### 1. Protocol Layer (UbxParser)

**Purpose:** Handle UBX binary protocol framing

**Responsibilities:**
- Detect sync bytes (0xB5 0x62)
- Extract message class and ID
- Read payload length
- Validate checksums
- Recover from corrupted data
- Handle multiple concatenated messages

**Key Design Decisions:**
- State machine for robust frame synchronization
- Non-blocking, byte-by-byte processing
- Checksum calculated incrementally
- Graceful error recovery (resets to SYNC1 state)

**Interface:**
```cpp
class UbxParser {
    bool parse_file(const std::string& filename);
    size_t parse_buffer(const uint8_t* data, size_t length);
    void set_observation_callback(ObservationCallback callback);
    void set_navigation_callback(NavigationCallback callback);
    void set_pvt_callback(PvtCallback callback);
};
```

### 2. Decoder Layer

**Purpose:** Convert binary payload to structured C++ objects

#### RawxDecoder

Decodes UBX-RXM-RAWX messages (raw multi-GNSS measurements).

**Input:** Binary payload (variable length)  
**Output:** `ObservationEpoch`

**Key Fields Decoded:**
- Header: rcvTow, week, leap seconds, number of measurements
- Per measurement: PR, CP, Doppler, CN0, validity flags, quality indicators

**Validation:**
- Check week number (reject week 0)
- Verify payload size matches expected size
- Apply validity rules (CP = -0.5 → invalid)
- Check standard deviation thresholds

#### SfrbxDecoder

Decodes UBX-RXM-SFRBX messages (navigation subframes).

**Input:** Binary payload  
**Output:** `NavigationMessage`

**Key Fields:**
- Constellation and satellite ID
- Signal ID
- GLONASS frequency channel
- Raw 32-bit navigation words

**Note:** Ephemeris decoding (interpreting the navigation words) is deferred to Phase 2.

#### NavPvtDecoder

Decodes UBX-NAV-PVT messages (receiver position/velocity/time).

**Input:** Binary payload (92 bytes)  
**Output:** `ReceiverPVT`

**Purpose:** Provides reference solution from receiver for comparison.

### 3. Data Model Layer

**Purpose:** Receiver-independent representation of GNSS data

#### Design Principles

1. **Explicit Validity:** Every measurement has a validity flag
2. **No Silent Conversions:** Store raw values (cycles, not metres for CP)
3. **SI Units:** Metres, seconds, hertz (not milliseconds or 0.1 Hz)
4. **Preserve All Information:** Quality indicators, lock time, standard deviations
5. **Constellation-Agnostic:** Support any GNSS system

#### Key Structures

**ObservationEpoch:**
- Time: GPS week + TOW
- Vector of observations
- Leap second information

**GnssObservation:**
- Constellation, satellite ID, signal ID, frequency channel
- Measurements: pseudorange, carrier phase, Doppler, CN0
- Validity flags (PR, CP, DO)
- Quality indicators (lock time, stdev)
- Half-cycle flags

**NavigationMessage:**
- Constellation, satellite ID, signal ID
- Raw navigation words (32-bit)
- Version and status fields

**ReceiverPVT:**
- Time (calendar format)
- Position (lat/lon/height)
- Velocity (NED frame)
- Accuracy estimates
- Fix type and satellite count

### 4. Signal Registry

**Purpose:** Map (constellation, signal ID) → signal properties

**Database:**
- GPS: L1C/A, L2C
- Galileo: E1-C, E1-B, E5a I/Q, E5b I/Q
- BeiDou: B1I D1/D2, B2I D1/D2
- GLONASS: G1 C/A, G2 C/A
- QZSS: L1C/A
- SBAS: L1C/A

**Special Handling:**
- GLONASS FDMA: Frequency depends on channel number
  - G1: 1602 MHz + 0.5625 MHz × channel
  - G2: 1246 MHz + 0.4375 MHz × channel
- Wavelength calculated from frequency: λ = c/f

**Interface:**
```cpp
std::optional<SignalInfo> get_signal_info(
    GnssConstellation constellation,
    uint8_t signal_id);

std::optional<double> get_carrier_frequency(
    GnssConstellation constellation,
    uint8_t signal_id,
    int8_t frequency_id = 0);
```

### 5. Analysis & Export Layer

**DiagnosticAnalyzer:**
- Collects statistics across all epochs
- Tracks time span, observation counts
- Counts by constellation and signal
- Identifies unique satellites

**CsvExporter:**
- Exports observations to CSV
- Exports navigation messages to CSV
- Exports PVT solutions to CSV
- Includes units and validity flags in headers

### 6. Application Layer

**gnss_inspect:**
- Command-line interface
- File parsing and statistics
- Epoch listing and detail view
- Satellite/signal summary
- CSV export options

## Data Flow

### Typical Processing Flow

```
UBX File
   │
   ↓ (binary stream)
UbxParser::parse_file()
   │
   ├─→ Frame sync (state machine)
   ├─→ Checksum validation
   ↓
UbxMessage {header, payload}
   │
   ├─→ RAWX? → RawxDecoder::decode()
   │             ↓
   │         ObservationEpoch → callback → app
   │
   ├─→ SFRBX? → SfrbxDecoder::decode()
   │              ↓
   │          NavigationMessage → callback → app
   │
   └─→ NAV-PVT? → NavPvtDecoder::decode()
                    ↓
                ReceiverPVT → callback → app
```

### Callback Pattern

Decoders don't return values directly. Instead:

```cpp
parser.set_observation_callback([](const ObservationEpoch& epoch) {
    // Process epoch
    analyzer.add_epoch(epoch);
    epochs.push_back(epoch);
});
```

**Benefits:**
- Streaming processing (no need to store all data)
- Decouples parsing from analysis
- Allows multiple consumers

## Error Handling Strategy

### Parse-Time Errors

**Checksum Failures:**
- Increment `checksum_failures` counter
- Reset state machine to SYNC1
- Continue parsing (do not abort)

**Malformed Messages:**
- Increment `malformed_messages` counter
- Reset state machine
- Log error details

**Sync Errors:**
- Increment `sync_errors` counter
- Reset state machine
- Continue searching for valid sync

**Result:** Parser never crashes on bad data. It reports errors and continues.

### Decode-Time Errors

**Invalid Payloads:**
- Return `std::nullopt` from decoder
- Message is counted but not processed

**Week 0 in RAWX:**
- Reject epoch (common GPS receiver issue)
- Return `std::nullopt`

**Unknown Constellations:**
- Skip observations from unknown systems
- Continue processing known systems

## Memory Management

**Stack Allocation:**
- Small, fixed-size structures
- GnssObservation, NavigationMessage, ReceiverPVT

**Heap Allocation (via std::vector):**
- Observation lists per epoch
- Navigation word storage
- Message buffers during parsing

**No Manual Memory Management:**
- All heap allocations via standard library containers
- RAII principles throughout
- No raw pointers (except for parsing byte buffers)

## Thread Safety

**Current Status:** Not thread-safe

**Design:**
- State stored in parser instance
- Callbacks are synchronous

**Thread-Safe Usage:**
- Use separate parser instance per thread
- Or protect parser with mutex

**Future:** Add thread-safe queue for multi-threaded applications

## Performance Considerations

### Parsing Performance

**Tested on:** 8.9 MB UBX file, 2977 epochs, 194,610 observations

**Result:** Parsing completes in <1 second on modern hardware

**Optimizations:**
- Incremental checksum calculation
- Reserve vector capacity when possible
- Minimal string operations
- Direct memory access (std::memcpy for endian-safe reads)

### Memory Footprint

**Per Epoch:**
- ~60 observations × ~100 bytes = ~6 KB
- Navigation messages: ~40 bytes each

**Total for 50-minute dataset:**
- ~18 MB for all epochs
- Reasonable for in-memory processing

## Extension Points

### Adding New Message Types

1. Define message constants in `ubx_parser.hpp`
2. Implement decoder in `src/ubx/`
3. Add decoder invocation in `UbxParser::process_message()`
4. Define callback type and storage
5. Update unit tests

### Adding New Constellations

1. Add to `GnssConstellation` enum
2. Update `ubx_gnss_id_to_constellation()` in decoders
3. Add signals to `SignalRegistry`
4. Update `constellation_to_string()`
5. Test with real data

### Adding Position Calculation (Phase 2)

Create new modules:
- `ephemeris_decoder.cpp`: Decode navigation words
- `satellite_position.cpp`: Calculate satellite XYZ
- `spp_solver.cpp`: Least-squares position estimation

These will consume the existing `ObservationEpoch` and `NavigationMessage` structures.

## Testing Strategy

### Unit Tests

Each decoder has dedicated tests:
- Valid inputs with known outputs
- Invalid inputs (too short, corrupted)
- Edge cases (zero measurements, week rollover)
- Boundary conditions

### Integration Tests

`gnss_inspect` serves as integration test:
- Parses real receiver data
- Validates against RTKLIB output
- Reports any discrepancies

### Validation

Comparison with RTKLIB:
- Generate RINEX with RTKLIB
- Export CSV with custom engine
- Compare numerically (exact match required)

## Design Rationale

### Why Not Inherit From RTKLIB Structures?

**Reason:** Clean separation and modern C++ practices

RTKLIB uses C-style structs with fixed-size arrays. Our design uses:
- `std::vector` for dynamic sizing
- `std::optional` for invalid/missing data
- Enum classes for type safety
- Explicit validity flags
- SI units consistently

### Why Store Carrier Phase in Cycles?

**Reason:** Avoid premature conversion

- Different signals have different wavelengths
- GLONASS wavelength depends on frequency channel
- Conversion to metres requires knowing the exact signal
- Better to store raw cycles and convert when needed

### Why Callbacks Instead of Return Values?

**Reason:** Streaming and flexibility

- Allows processing data as it arrives
- No need to accumulate all epochs in memory
- Multiple consumers can listen to same stream
- Easier to add logging, filtering, etc.

### Why Separate Decoders?

**Reason:** Single Responsibility Principle

Each decoder handles one message type:
- Simpler to test
- Easier to maintain
- Can be used independently
- Clear separation of concerns

## Future Architecture Changes

### Phase 2: Ephemeris Module

```
NavigationMessage
   ↓
EphemerisDecoder
   ↓
BroadcastEphemeris (Keplerian elements)
   ↓
SatellitePositionCalculator
   ↓
Satellite ECEF coordinates
```

### Phase 3: SPP Module

```
ObservationEpoch + BroadcastEphemeris
   ↓
PseudorangeCorrector (sat clock, iono, tropo)
   ↓
LeastSquaresSolver
   ↓
ReceiverPosition (ECEF)
```

### Phase 4-5: Kalman Filter and RTK

New modules for:
- Extended Kalman Filter
- Double-difference formation
- Ambiguity resolution
- Base station data handling

---

**Architecture designed for:**
- Correctness over performance
- Testability and maintainability  
- Clear separation of concerns
- Extensibility for future phases
