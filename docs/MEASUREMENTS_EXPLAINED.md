# GNSS Measurements Explained

## Understanding the Parser Output

When you run `gnss_parser`, you see this table:

```
Const     SV   SigID   Pseudorange(m)   V  CarrierPhase(cy)    V  Doppler(Hz) V  CN0(dBHz)Signal
```

This document explains **what each parameter means** and **how to calculate position from this data**.

---

## Parameters Explained

### 1. Const (Constellation)

**What it is:** The GNSS system the satellite belongs to.

**Values:**
- `GPS` - USA Global Positioning System
- `Galileo` - European Union system
- `BeiDou` - Chinese system
- `GLONASS` - Russian system
- `QZSS` - Japanese Quasi-Zenith Satellite System
- `SBAS` - Satellite-Based Augmentation System

**Why it matters:**
- Different systems have different time scales (GPS time, GLONASS time, etc.)
- Different orbital parameters
- Different signal frequencies
- Position calculation must account for these differences

**Example:**
```
GPS       4    ...  ← GPS satellite PRN 4
Galileo   2    ...  ← Galileo satellite E02
```

---

### 2. SV (Satellite Vehicle)

**What it is:** The satellite identifier within its constellation.

**Also called:** PRN (Pseudo-Random Noise) number or Slot number

**Range:**
- GPS: 1-32
- Galileo: 1-36
- BeiDou: 1-63
- GLONASS: 1-24 (slot numbers)
- QZSS: 1-7

**Why it matters:**
- Uniquely identifies which satellite sent the signal
- Used to look up satellite ephemeris (orbital parameters)
- Different satellites have different positions at any time

**Example:**
```
GPS       4    ...  ← This is GPS satellite #4
GPS       27   ...  ← This is GPS satellite #27
```

---

### 3. SigID (Signal Identifier)

**What it is:** Identifies which frequency band and signal code.

**Common values:**
- `0` - Primary signal (L1C/A for GPS, E1 for Galileo, B1I for BeiDou)
- `3` - Secondary signal (L2C for GPS)
- `6` - L5/E5a signal

**Why it matters:**
- Different frequencies have different wavelengths
- Used for dual-frequency positioning (removes ionospheric delay)
- Some signals are more precise than others

**Signal frequencies:**
```
GPS SigID 0 (L1C/A):  1575.42 MHz, wavelength 0.1903 m
GPS SigID 3 (L2C):    1227.60 MHz, wavelength 0.2442 m
Galileo SigID 0 (E1): 1575.42 MHz, wavelength 0.1903 m
Galileo SigID 6 (E5a):1176.45 MHz, wavelength 0.2548 m
```

**Example:**
```
GPS       4    0    ...  ← GPS SV4, L1 signal
GPS       4    3    ...  ← GPS SV4, L2 signal (same satellite, different frequency)
```

---

### 4. Pseudorange (metres)

**What it is:** The measured distance between satellite and receiver.

**Formula:**
```
Pseudorange = (Signal travel time) × (Speed of light)
```

**Why "pseudo"?** Because it includes errors:
```
Pseudorange = True_Distance + c×dt_receiver + c×dt_satellite + Atmospheric_delays + Noise
```

Where:
- `True_Distance` = Geometric distance (what we want)
- `c×dt_receiver` = Receiver clock offset (converted to distance)
- `c×dt_satellite` = Satellite clock offset
- `Atmospheric_delays` = Ionosphere + Troposphere
- `Noise` = Measurement errors

**Typical values:** 20,000,000 m to 26,000,000 m (20,000-26,000 km)

**Accuracy:** ~30 cm (for code-based measurements)

**Why it matters:**
- **This is the PRIMARY measurement for position calculation**
- With 4+ satellites, we can solve for receiver position (X,Y,Z) and clock offset

**Example:**
```
GPS       4    0    21743857.636   Y  ← 21,743 km pseudorange
```

**Visual explanation:**
```
         Satellite (at 20,200 km altitude)
              *
             /|
            / |
           /  | True geometric distance
          /   |
         /    |
        /     |
       /      |
      /       |
     *--------+
  Receiver  Clock offset adds apparent distance

Pseudorange = √(distance²) + clock_errors + atmosphere
```

---

### 5. V (Validity Flag for Pseudorange)

**What it is:** Indicates if the pseudorange measurement is valid.

**Values:**
- `Y` - Valid, usable measurement
- `N` - Invalid, do not use

**Why invalid?**
- Signal too weak
- Just started tracking
- Cycle slip occurred
- Receiver marked it unreliable

**Usage:**
- **Only use measurements with V=Y for position calculation**
- Invalid measurements should be discarded

---

### 6. CarrierPhase (cycles)

**What it is:** The phase of the carrier wave, measured in cycles.

**More precise than pseudorange!**
- Pseudorange accuracy: ~30 cm
- Carrier phase accuracy: ~1-2 mm (when ambiguity resolved)

**Formula:**
```
CarrierPhase = (Distance / Wavelength) + Integer_Ambiguity + Phase_offset
```

**The challenge:** We don't know the integer ambiguity (how many full wavelengths).

**Example values:** 114,264,745 cycles for GPS L1

**Converting to distance:**
```
Distance = CarrierPhase × Wavelength
         = 114,264,745 cycles × 0.1903 m
         = 21,743,858 metres
```

**Why it matters:**
- Used for high-precision positioning (RTK)
- Requires continuous tracking (cycle slips are bad)
- Much more precise than pseudorange
- Need to resolve integer ambiguity

**Usage scenarios:**
- **Single Point Positioning (SPP):** Use pseudorange only (simpler, ~meter accuracy)
- **RTK (Real-Time Kinematic):** Use carrier phase (complex, cm accuracy)

---

### 7. V (Validity Flag for Carrier Phase)

**What it is:** Indicates if carrier phase is valid.

**Values:**
- `Y` - Valid, continuous tracking
- `N` - Invalid or not tracked

**Why invalid?**
- Signal too weak (CN0 < ~35 dB-Hz)
- Lost lock (cycle slip)
- Just started tracking satellite
- Receiver marked phase unreliable

**Critical for RTK:** Carrier phase must be continuously valid, any break requires re-initialization.

---

### 8. Doppler (Hz)

**What it is:** The frequency shift due to relative motion between satellite and receiver.

**Caused by:** Doppler effect (same as sound pitch change when vehicle passes)

**Formula:**
```
Doppler = -(Relative_Velocity / Speed_of_light) × Carrier_Frequency
```

**Sign convention:**
- Negative: Satellite moving away (frequency decreasing)
- Positive: Satellite approaching (frequency increasing)

**Typical range:** ±5,000 Hz

**Why it matters:**
- Can estimate receiver **velocity** from Doppler
- Helps predict satellite motion
- Used in Kalman filters for positioning
- Helps detect cycle slips

**Example:**
```
GPS       4    0    ...  -1714.171   Y  ← Moving away at ~327 m/s radial velocity
GPS       8    0    ...   986.858    Y  ← Approaching at ~188 m/s radial velocity
```

**Velocity calculation:**
```
Radial_Velocity = -Doppler × (Speed_of_light / Carrier_Frequency)
                = -Doppler × Wavelength
```

---

### 9. V (Validity Flag for Doppler)

**What it is:** Indicates if Doppler measurement is valid.

**Usually:** Valid whenever pseudorange or carrier phase is being tracked.

---

### 10. CN0 (Carrier-to-Noise Density Ratio)

**What it is:** Signal strength measurement in dB-Hz.

**Interpretation:**
```
< 30 dB-Hz:  Very weak (indoors, heavy obstruction)
30-35 dB-Hz: Weak signal
35-40 dB-Hz: Fair signal
40-45 dB-Hz: Good signal (typical outdoors)
> 45 dB-Hz:  Excellent signal (clear sky view)
```

**Why it matters:**
- **Weight measurements in position calculation** (higher CN0 = more reliable)
- Detect multipath (fluctuating CN0)
- Assess tracking quality
- Filter out weak satellites

**Usage in position calculation:**
```
Weight = 1 / (measurement_variance)
       ≈ function(CN0)

High CN0 → Low variance → High weight in least-squares
```

**Example:**
```
GPS       4    0    ...  45.0   ← Excellent signal
GPS       26   0    ...  29.0   ← Weak signal (might exclude from solution)
```

---

### 11. Signal (Signal Name)

**What it is:** Human-readable signal identification.

**Examples:**
- `GPS L1C/A` - GPS coarse/acquisition code on L1
- `GPS L2 CL` - GPS civil L2 code
- `GAL E1 C` - Galileo E1 C component
- `GAL E5a Q` - Galileo E5a Q component
- `BDS B1I D1` - BeiDou B1I data component
- `GLO G1 C/A` - GLONASS G1 coarse/acquisition

**Why it matters:**
- Identifies the specific signal for frequency lookup
- Different signals have different characteristics
- Used to compute wavelength for carrier phase conversion

---

## Complete Example Row Explained

```
GPS       4    0    21743857.636   Y  114264745.094   Y  -1714.171   Y  45.0   GPS L1C/A
```

**Breaking it down:**

1. **GPS** - US GPS constellation
2. **4** - Satellite PRN 4
3. **0** - Signal ID 0 (L1 C/A signal, 1575.42 MHz)
4. **21743857.636 m** - Pseudorange (21,743 km)
5. **Y** - Pseudorange is valid
6. **114264745.094 cycles** - Carrier phase
7. **Y** - Carrier phase is valid (continuous tracking)
8. **-1714.171 Hz** - Doppler shift (satellite moving away)
9. **Y** - Doppler is valid
10. **45.0 dB-Hz** - Excellent signal strength
11. **GPS L1C/A** - L1 coarse/acquisition signal

**What this tells us:**
- Strong, healthy signal from GPS satellite 4
- Both code and carrier phase are being tracked
- Distance is ~21,743 km (includes clock errors!)
- Satellite is moving away from receiver
- High quality measurement (CN0 = 45 dB-Hz)

---

## How Position is Calculated From This Data

### Overview: The Big Picture

```
Step 1: Get Measurements (DONE! ✅)
   ↓
Step 2: Get Satellite Positions (TODO - Phase 2)
   ↓
Step 3: Apply Corrections (TODO - Phase 2)
   ↓
Step 4: Solve for Receiver Position (TODO - Phase 2)
   ↓
Step 5: Convert to Lat/Lon/Height (TODO - Phase 2)
   ↓
Step 6: Generate NMEA Output (TODO - Phase 3)
```

---

## Detailed: Position Calculation Algorithm

### Step 1: Get Measurements ✅ (CURRENT PHASE)

**Input:** UBX binary data  
**Process:** Parse with our tool  
**Output:** Pseudoranges from multiple satellites

```
Satellite 1: PR₁ = 21,743,857.636 m
Satellite 2: PR₂ = 23,243,097.748 m
Satellite 3: PR₃ = 24,345,425.795 m
Satellite 4: PR₄ = 22,357,709.707 m
```

---

### Step 2: Get Satellite Positions 🚧 (NEXT PHASE)

**Need:** Satellite coordinates (X, Y, Z) at measurement time

**How to get them:**

#### A. Decode Ephemeris from Navigation Messages

From SFRBX messages (navigation data), extract:
- Orbital parameters (Keplerian elements)
- Satellite clock corrections
- Time of ephemeris

**Keplerian Elements:**
```
a     - Semi-major axis (orbit size)
e     - Eccentricity (orbit shape)
i₀    - Inclination angle
Ω₀    - Right ascension of ascending node
ω     - Argument of perigee
M₀    - Mean anomaly
n     - Mean motion
```

#### B. Calculate Satellite Position

**Algorithm:**
```
1. Compute mean anomaly at measurement time:
   M = M₀ + n × (t - t_oe)

2. Solve Kepler's equation for eccentric anomaly E:
   M = E - e × sin(E)    [iterative solution]

3. Calculate true anomaly ν:
   ν = atan2(√(1-e²) × sin(E), cos(E) - e)

4. Calculate argument of latitude:
   u = ν + ω

5. Compute orbital radius:
   r = a × (1 - e × cos(E))

6. Convert to orbital plane coordinates:
   x' = r × cos(u)
   y' = r × sin(u)

7. Rotate to Earth-Centered Earth-Fixed (ECEF):
   X_sat = x' × cos(Ω) - y' × cos(i) × sin(Ω)
   Y_sat = x' × sin(Ω) + y' × cos(i) × cos(Ω)
   Z_sat = y' × sin(i)
```

**Result:** Satellite position (X_sat, Y_sat, Z_sat) in ECEF coordinates

**Example:**
```
GPS SV 4 position at TOW 207960.005:
X_sat = 15,234,567.123 m
Y_sat = -18,456,789.456 m
Z_sat = 12,345,678.789 m
```

---

### Step 3: Apply Corrections 🚧

**The pseudorange needs corrections:**

#### A. Satellite Clock Correction

From ephemeris:
```
dt_sat = a₀ + a₁(t - t_oc) + a₂(t - t_oc)²
Corrected_PR = Measured_PR + c × dt_sat
```

#### B. Ionospheric Delay

**Single-frequency (simple):**
```
Iono_delay = Klobuchar_model(elevation, azimuth, time, receiver_pos)
Corrected_PR = Corrected_PR - Iono_delay
```

**Dual-frequency (better):**
```
Iono_free_PR = (f₁² × PR₁ - f₂² × PR₂) / (f₁² - f₂²)
```

#### C. Tropospheric Delay

```
Tropo_delay = Saastamoinen_model(elevation, pressure, temperature, humidity)
Corrected_PR = Corrected_PR - Tropo_delay
```

#### D. Relativistic Correction

```
dt_rel = F × e × √a × sin(E)    [F = -4.442807633e-10]
Corrected_PR = Corrected_PR + c × dt_rel
```

---

### Step 4: Solve for Receiver Position 🚧

**Mathematical Problem:**

We have 4+ equations (one per satellite):

```
PR₁ = √[(X_sat1 - X_rcv)² + (Y_sat1 - Y_rcv)² + (Z_sat1 - Z_rcv)²] + c×dt_rcv
PR₂ = √[(X_sat2 - X_rcv)² + (Y_sat2 - Y_rcv)² + (Z_sat2 - Z_rcv)²] + c×dt_rcv
PR₃ = √[(X_sat3 - X_rcv)² + (Y_sat3 - Y_rcv)² + (Z_sat3 - Z_rcv)²] + c×dt_rcv
PR₄ = √[(X_sat4 - X_rcv)² + (Y_sat4 - Y_rcv)² + (Z_sat4 - Z_rcv)²] + c×dt_rcv
```

**Unknowns (4 total):**
- X_rcv, Y_rcv, Z_rcv (receiver position)
- dt_rcv (receiver clock offset)

**Solution Method: Least-Squares Iteration**

```
1. Start with initial guess (X₀, Y₀, Z₀, dt₀)
   [Can use rough lat/lon or previous position]

2. Linearize equations around guess:
   ΔPR = H × Δx
   
   Where:
   H = Jacobian matrix (geometry matrix)
   Δx = [ΔX, ΔY, ΔZ, c×Δdt] (corrections to apply)

3. Solve normal equations:
   Δx = (Hᵀ × W × H)⁻¹ × Hᵀ × W × ΔPR
   
   Where W = weight matrix (diagonal, based on CN0)

4. Update position:
   X = X₀ + ΔX
   Y = Y₀ + ΔY
   Z = Z₀ + ΔZ
   dt = dt₀ + Δdt

5. Iterate until convergence (typically 3-5 iterations)
   |ΔX|² + |ΔY|² + |ΔZ|² < 0.01 m²
```

**Geometry Matrix (H):**
```
For each satellite i:
    l_i = unit vector from receiver to satellite
    
    H[i] = [-l_ix, -l_iy, -l_iz, 1]
```

**Weight Matrix (W):**
```
W[i,i] = 1 / σ²ᵢ

Where σᵢ depends on:
- CN0 (signal strength)
- Elevation angle (low elevation = more error)
- Multipath indicators
```

**Result:** Receiver position in ECEF coordinates
```
X_rcv = 4,567,123.456 m
Y_rcv = 1,234,567.890 m
Z_rcv = 4,321,098.765 m
dt_rcv = 0.000123456 s (clock offset)
```

---

### Step 5: Convert to Geodetic Coordinates 🚧

**Convert ECEF (X, Y, Z) to Latitude, Longitude, Height**

**Algorithm (Bowring's method):**

```
1. Calculate longitude:
   λ = atan2(Y, X)

2. Calculate initial latitude guess:
   p = √(X² + Y²)
   φ = atan2(Z, p × (1 - e²))

3. Iterate to refine latitude:
   N = a / √(1 - e² × sin²(φ))
   h = p / cos(φ) - N
   φ_new = atan2(Z, p × (1 - e² × N/(N + h)))
   
   Repeat until φ converges

4. Calculate height:
   h = p / cos(φ) - N
```

**Constants (WGS84 ellipsoid):**
```
a = 6378137.0 m          (semi-major axis)
f = 1/298.257223563      (flattening)
e² = 2f - f²             (eccentricity squared)
```

**Result:**
```
Latitude  = 48.1234567° N
Longitude = 11.5678901° E
Height    = 521.234 m (above WGS84 ellipsoid)
```

---

### Step 6: Calculate Dilution of Precision (DOP) 🚧

**Quality metrics for the solution:**

```
Q = (Hᵀ × H)⁻¹    [Covariance matrix]

GDOP = √(Q₁₁ + Q₂₂ + Q₃₃ + Q₄₄)  [Geometric DOP]
PDOP = √(Q₁₁ + Q₂₂ + Q₃₃)        [Position DOP]
HDOP = √(Q₁₁ + Q₂₂)              [Horizontal DOP]
VDOP = √Q₃₃                       [Vertical DOP]
TDOP = √Q₄₄                       [Time DOP]
```

**Interpretation:**
```
DOP < 2:  Excellent
DOP 2-5:  Good
DOP 5-10: Moderate
DOP > 10: Poor (consider not using solution)
```

**Why DOP matters:** Low DOP = good satellite geometry = accurate position

---

### Step 7: Generate NMEA Sentences 🚧 (PHASE 3)

**Format the position as NMEA-0183 sentences**

#### Example: GPGGA (Global Positioning System Fix Data)

```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
```

**Fields:**
```
$GPGGA       - Message ID
123519       - UTC time (12:35:19)
4807.038,N   - Latitude (48°07.038' North)
01131.000,E  - Longitude (11°31.000' East)
1            - Fix quality (1 = GPS fix)
08           - Number of satellites
0.9          - HDOP
545.4,M      - Altitude above MSL
46.9,M       - Geoid separation
,,           - DGPS fields (empty)
*47          - Checksum
```

#### Example: GPRMC (Recommended Minimum)

```
$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
```

**Fields:**
```
$GPRMC       - Message ID
123519       - UTC time
A            - Status (A=active, V=void)
4807.038,N   - Latitude
01131.000,E  - Longitude
022.4        - Speed over ground (knots)
084.4        - Track angle (degrees)
230394       - Date (23 March 1994)
003.1,W      - Magnetic variation
*6A          - Checksum
```

---

## Summary: The Complete Process

### What We Have Now ✅

```python
# Phase 1: Parsing (DONE)
measurements = parse_ubx_file("rover.ubx")
# Returns: pseudorange, carrier_phase, doppler, satellite_id, etc.
```

### What We Need to Build 🚧

```python
# Phase 2: Position Calculation (TODO)
def calculate_position(measurements, navigation_messages):
    # Step 1: Decode ephemeris from navigation messages
    ephemeris = decode_ephemeris(navigation_messages)
    
    # Step 2: Calculate satellite positions
    sat_positions = []
    for sat_id in measurements.satellites:
        pos = calculate_satellite_position(
            ephemeris[sat_id],
            measurements.time
        )
        sat_positions.append(pos)
    
    # Step 3: Apply corrections
    corrected_pseudoranges = []
    for pr, sat in zip(measurements.pseudoranges, sat_positions):
        pr_corr = apply_satellite_clock_correction(pr, ephemeris)
        pr_corr = apply_ionospheric_correction(pr_corr, sat, receiver_pos_estimate)
        pr_corr = apply_tropospheric_correction(pr_corr, sat, receiver_pos_estimate)
        corrected_pseudoranges.append(pr_corr)
    
    # Step 4: Solve for position (least-squares)
    receiver_position_ecef = least_squares_solve(
        corrected_pseudoranges,
        sat_positions,
        initial_guess
    )
    
    # Step 5: Convert to geodetic
    lat, lon, height = ecef_to_geodetic(receiver_position_ecef)
    
    return lat, lon, height

# Phase 3: NMEA Generation (TODO)
def generate_nmea(lat, lon, height, velocity, time, num_sats, hdop):
    gga = format_gpgga(lat, lon, height, time, num_sats, hdop)
    rmc = format_gprmc(lat, lon, velocity, time)
    return gga, rmc
```

---

## Key Takeaways

### Current Status

✅ **We can extract measurements** (pseudorange, phase, Doppler)  
✅ **We have validated data** from real receiver  
✅ **Parser is production-ready**

### Next Steps

🚧 **Decode ephemeris** (satellite orbital parameters)  
🚧 **Calculate satellite positions** (Keplerian orbit math)  
🚧 **Implement corrections** (clocks, atmosphere)  
🚧 **Solve least-squares** (position calculation)  
🚧 **Generate NMEA** (standard output format)

### Complexity

- **Phase 1 (Parsing):** Medium - Binary protocol handling ✅
- **Phase 2 (Position):** Hard - Orbital mechanics, corrections, numerical methods 🚧
- **Phase 3 (NMEA):** Easy - String formatting 🚧

---

## References

For implementing Phase 2, you'll need:

1. **GPS Interface Control Document (IS-GPS-200)** - Ephemeris format, corrections
2. **RTKLIB Source Code** - Reference implementation
3. **"A Software-Defined GPS and Galileo Receiver"** by Kai Borre - Excellent algorithms
4. **"Global Positioning System: Signals, Measurements, and Performance"** by Misra & Enge

---

**Document Version:** 1.0  
**Last Updated:** October 2026  
**Status:** Phase 1 Complete, Phase 2 Pending
