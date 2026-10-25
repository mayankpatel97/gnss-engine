# UBX Message Support

## Overview

This document describes the UBX protocol messages supported by the GNSS engine and their implementation details.

## UBX Protocol Basics

### Message Structure

```
┌──────┬──────┬───────┬──────┬────────┬─────────┬─────────┬─────────┐
│ B5   │ 62   │ CLASS │ ID   │ LEN_L  │ LEN_H   │ PAYLOAD │ CK_A CK_B│
│(sync)│(sync)│       │      │        │         │         │(checksum)│
└──────┴──────┴───────┴──────┴────────┴─────────┴─────────┴─────────┘
  1      1       1       1       1        1       N bytes     1    1
```

### Checksum Algorithm

Fletcher's checksum over CLASS, ID, LENGTH, and PAYLOAD:

```cpp
CK_A = 0
CK_B = 0
for each byte in (CLASS, ID, LEN_L, LEN_H, PAYLOAD):
    CK_A = CK_A + byte
    CK_B = CK_B + CK_A
```

### Endianness

All multi-byte values in UBX messages are **little-endian**.

## Supported Messages

### UBX-RXM-RAWX (0x02 0x15)

**Purpose:** Multi-GNSS raw measurement data

**When Generated:** Every measurement epoch (typically 1 Hz)

**Payload Structure:**

```
Offset | Size | Type   | Name       | Description
-------|------|--------|------------|---------------------------
0      | 8    | R8     | rcvTow     | Receiver time of week (s)
8      | 2    | U2     | week       | GPS week number
10     | 1    | I1     | leapS      | GPS-UTC leap seconds
11     | 1    | U1     | numMeas    | Number of measurements
12     | 1    | U1     | recStat    | Receiver tracking status
13     | 1    | U1     | version    | Message version (1)
14     | 2    | U1[2]  | reserved   | Reserved
16     | 32*N | struct | meas[]     | Measurement blocks
```

**Measurement Block (32 bytes each):**

```
Offset | Size | Type | Name     | Description                  | Units | Scale
-------|------|------|----------|------------------------------|-------|-------
0      | 8    | R8   | prMes    | Pseudorange measurement      | m     | 1
8      | 8    | R8   | cpMes    | Carrier phase measurement    | cycles| 1
16     | 4    | R4   | doMes    | Doppler measurement          | Hz    | 1
20     | 1    | U1   | gnssId   | GNSS identifier              | -     | 1
21     | 1    | U1   | svId     | Satellite identifier         | -     | 1
22     | 1    | U1   | sigId    | Signal identifier (new)      | -     | 1
23     | 1    | U1   | freqId   | GLONASS freq slot + 7        | -     | 1
24     | 2    | U2   | locktime | Carrier phase lock time      | ms    | 1
26     | 1    | U1   | cno      | Carrier-to-noise density     | dB-Hz | 1
27     | 1    | X1   | prStdev  | Pseudorange stdev (bits 0-3) | -     | 2^n
28     | 1    | X1   | cpStdev  | Carrier phase stdev (bits 0-3)| -    | 2^n
29     | 1    | X1   | doStdev  | Doppler stdev (bits 0-3)     | -     | 2^n
30     | 1    | X1   | trkStat  | Tracking status flags        | -     | 1
31     | 1    | U1   | reserved | Reserved                     | -     | 1
```

**GNSS Identifier (gnssId):**

| Value | GNSS System |
|-------|-------------|
| 0 | GPS |
| 1 | SBAS |
| 2 | Galileo |
| 3 | BeiDou |
| 4 | IMES |
| 5 | QZSS |
| 6 | GLONASS |

**Tracking Status Flags (trkStat):**

| Bit | Meaning |
|-----|---------|
| 0 | Pseudorange valid |
| 1 | Carrier phase valid |
| 2 | Half cycle valid |
| 3 | Half cycle subtracted from phase |

**Implementation Notes:**

1. **Week 0 Check:** Reject messages with week = 0 (common error condition)
2. **Carrier Phase Validity:** Additional check for cpMes == -0.5 (invalid marker)
3. **Standard Deviation Threshold:** cpStdev > 5 indicates unreliable phase
4. **GLONASS Frequency:** freqId stores channel + 7, so actual channel = freqId - 7
5. **Doppler Validity:** Set valid if either PR or CP is being tracked

**Supported Signals (sigId):**

- **GPS:** 0 (L1C/A), 3 (L2 CL), 4 (L2 CM), 6 (L5 I), 7 (L5 Q)
- **Galileo:** 0 (E1 C), 1 (E1 B), 5 (E5a I), 6 (E5a Q), 7 (E5b I), 8 (E5b Q)
- **BeiDou:** 0 (B1I D1), 1 (B1I D2), 2 (B2I D1), 3 (B2I D2)
- **GLONASS:** 0 (G1 C/A), 2 (G2 C/A)
- **QZSS:** 0 (L1C/A), 5 (L5)
- **SBAS:** 0 (L1C/A)

### UBX-RXM-SFRBX (0x02 0x13)

**Purpose:** Broadcast navigation data subframe

**When Generated:** When navigation data is received from satellites

**Payload Structure:**

```
Offset | Size | Type   | Name     | Description
-------|------|--------|----------|-----------------------------
0      | 1    | U1     | gnssId   | GNSS identifier
1      | 1    | U1     | svId     | Satellite identifier
2      | 1    | U1     | reserved1| Reserved
3      | 1    | U1     | freqId   | GLONASS frequency slot + 7
4      | 1    | U1     | numWords | Number of data words
5      | 1    | U1     | chn      | Channel number
6      | 1    | U1     | version  | Message version (2)
7      | 1    | U1     | reserved2| Reserved
8      | 4*N  | U4[]   | dwrd[]   | Data words (32-bit each)
```

**Implementation Notes:**

1. **Raw Storage:** Navigation words are stored without interpretation
2. **Ephemeris Decoding:** Deferred to Phase 2
3. **Signal ID:** Not provided in SFRBX; inferred as 0 for compatibility
4. **GLONASS Frequency:** Same encoding as RAWX (freqId - 7)

**Navigation Word Formats by Constellation:**

- **GPS:** 30-bit words (LSB aligned in 32-bit container)
- **Galileo:** I/NAV or F/NAV pages
- **BeiDou:** D1/D2 navigation messages
- **GLONASS:** 85-bit strings (multiple words)
- **QZSS:** Same as GPS

### UBX-NAV-PVT (0x01 0x07)

**Purpose:** Navigation position/velocity/time solution

**When Generated:** Every navigation epoch

**Payload Structure (92 bytes):**

```
Offset | Size | Type | Name      | Description              | Units | Scale
-------|------|------|-----------|--------------------------|-------|-------
0      | 4    | U4   | iTOW      | GPS time of week         | ms    | 1
4      | 2    | U2   | year      | Year (UTC)               | y     | 1
6      | 1    | U1   | month     | Month (1-12)             | -     | 1
7      | 1    | U1   | day       | Day (1-31)               | -     | 1
8      | 1    | U1   | hour      | Hour (0-23)              | h     | 1
9      | 1    | U1   | min       | Minute (0-59)            | min   | 1
10     | 1    | U1   | sec       | Second (0-60)            | s     | 1
11     | 1    | X1   | valid     | Validity flags           | -     | 1
12     | 4    | U4   | tAcc      | Time accuracy estimate   | ns    | 1
16     | 4    | I4   | nano      | Fraction of second       | ns    | 1
20     | 1    | U1   | fixType   | GNSS fix type            | -     | 1
21     | 1    | X1   | flags     | Fix status flags         | -     | 1
22     | 1    | X1   | flags2    | Additional flags         | -     | 1
23     | 1    | U1   | numSV     | Number of satellites     | -     | 1
24     | 4    | I4   | lon       | Longitude                | deg   | 1e-7
28     | 4    | I4   | lat       | Latitude                 | deg   | 1e-7
32     | 4    | I4   | height    | Height above ellipsoid   | mm    | 1
36     | 4    | I4   | hMSL      | Height above MSL         | mm    | 1
40     | 4    | U4   | hAcc      | Horizontal accuracy      | mm    | 1
44     | 4    | U4   | vAcc      | Vertical accuracy        | mm    | 1
48     | 4    | I4   | velN      | North velocity           | mm/s  | 1
52     | 4    | I4   | velE      | East velocity            | mm/s  | 1
56     | 4    | I4   | velD      | Down velocity            | mm/s  | 1
60     | 4    | I4   | gSpeed    | Ground speed             | mm/s  | 1
64     | 4    | I4   | headMot   | Heading of motion        | deg   | 1e-5
68     | 4    | U4   | sAcc      | Speed accuracy           | mm/s  | 1
72     | 4    | U4   | headAcc   | Heading accuracy         | deg   | 1e-5
76     | 2    | U2   | pDOP      | Position DOP             | -     | 0.01
78     | 6    | U1[6]| reserved  | Reserved                 | -     | 1
84     | 4    | I4   | headVeh   | Heading of vehicle       | deg   | 1e-5
88     | 2    | I2   | magDec    | Magnetic declination     | deg   | 1e-2
90     | 2    | U2   | magAcc    | Magnetic declination acc | deg   | 1e-2
```

**Fix Type Values:**

| Value | Meaning |
|-------|---------|
| 0 | No fix |
| 1 | Dead reckoning only |
| 2 | 2D fix |
| 3 | 3D fix |
| 4 | GNSS + dead reckoning |
| 5 | Time only fix |

**Carrier Solution (flags bits 6-7):**

| Value | Meaning |
|-------|---------|
| 0 | No carrier solution |
| 1 | Float solution |
| 2 | Fixed solution |

**Implementation Notes:**

1. **Optional:** NAV-PVT is not required for raw data processing
2. **Reference Only:** Used to compare against custom position solution
3. **Unit Conversions:** All scaled values converted to SI units (degrees, metres, m/s)

## Message Classes Not Implemented

### Phase 1 Scope

The following message types are intentionally **not** implemented in Phase 1:

- **UBX-NAV-*** (except NAV-PVT): Navigation solution messages
- **UBX-CFG-***: Configuration messages
- **UBX-MON-***: Monitoring messages
- **UBX-AID-***: AssistNow aiding messages
- **UBX-TIM-***: Timing messages
- **UBX-ESF-***: External sensor fusion messages
- **UBX-MGA-***: Multiple GNSS assistance messages
- **UBX-LOG-***: Logging messages
- **UBX-SEC-***: Security messages
- **UBX-HNR-***: High-rate navigation messages

These may be added in future phases as needed.

## Parsing Implementation

### State Machine

The parser uses a finite state machine to handle frame synchronization:

```
SYNC1 ──→ SYNC2 ──→ CLASS ──→ ID ──→ LENGTH_LSB ──→ LENGTH_MSB
                                                          │
                                                          ↓
          CHECKSUM_B ←── CHECKSUM_A ←── PAYLOAD ←────────┘
                │
                └──→ Message Complete ──→ SYNC1
```

**Error Recovery:**
- Any checksum failure → reset to SYNC1
- Malformed message → reset to SYNC1
- Invalid sync byte → continue to SYNC1

### Byte Extraction Helpers

```cpp
// Little-endian extraction
uint8_t  extract_u1(const uint8_t* p);
uint16_t extract_u2(const uint8_t* p);
uint32_t extract_u4(const uint8_t* p);
int8_t   extract_i1(const uint8_t* p);
int16_t  extract_i2(const uint8_t* p);
int32_t  extract_i4(const uint8_t* p);
float    extract_r4(const uint8_t* p);
double   extract_r8(const uint8_t* p);
```

These use `std::memcpy` for endian-safe, alignment-safe extraction.

## Data Validation Rules

### RAWX Message

1. **Minimum Size:** 16 + 32 × numMeas bytes
2. **Week Number:** Must be > 0 (week 0 is invalid)
3. **Carrier Phase:** cpMes == -0.5 → mark invalid
4. **CP Standard Deviation:** cpStdev > 5 → mark invalid
5. **Tracking Status:** Check bits 0-1 for PR/CP validity

### SFRBX Message

1. **Minimum Size:** 8 + 4 × numWords bytes
2. **Constellation:** Must map to known GNSS system
3. **Navigation Words:** Must have at least 1 word

### NAV-PVT Message

1. **Minimum Size:** 84 bytes (some versions have 92)
2. **Validity Flags:** Check bit 0 (validDate), bit 1 (validTime)
3. **Fix Type:** Values 0-5 are standard

## Message Frequency

Typical message rates from ZED-F9P at 1 Hz navigation:

| Message | Rate | Count/min |
|---------|------|-----------|
| RXM-RAWX | 1 Hz | 60 |
| RXM-SFRBX | Variable | 300-600 |
| NAV-PVT | 1 Hz | 60 |

**Note:** SFRBX rate depends on:
- Number of satellites in view
- Navigation data update rate (varies by constellation)
- Signal strength and tracking quality

## Testing

### Test Vectors

Unit tests include:
- Valid messages with known outputs
- Invalid checksums
- Truncated messages
- Zero-length payloads
- Week 0 conditions
- Extreme coordinate values
- Unknown constellation IDs

### Real Data Validation

Validated against:
- ZED-F9P recording: 8.9 MB, 2977 epochs
- Compared with RTKLIB RINEX conversion
- Exact numerical match for all measurements

## References

1. **u-blox ZED-F9P Interface Description**  
   Document Number: UBX-18010854-R08  
   Dated: July 22, 2020

2. **RTKLIB ublox.c Implementation**  
   Path: `RTKLIB/src/rcv/ublox.c`  
   Used as reference for UBX decoding

3. **UBX Protocol Specification**  
   Included in u-blox receiver manuals

---

**Document Version:** 1.0  
**Last Updated:** December 2026  
**Implementation Status:** Phase 1 Complete
