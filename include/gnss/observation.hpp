#ifndef GNSS_OBSERVATION_HPP
#define GNSS_OBSERVATION_HPP

#include <cstdint>
#include <vector>
#include <string>

namespace gnss {

/**
 * GNSS constellation identifiers.
 * Based on u-blox UBX protocol specification.
 */
enum class GnssConstellation : uint8_t {
    GPS = 0,
    SBAS = 1,
    Galileo = 2,
    BeiDou = 3,
    IMES = 4,
    QZSS = 5,
    GLONASS = 6,
    Unknown = 255
};

/**
 * Convert constellation enum to string.
 */
const char* constellation_to_string(GnssConstellation constellation);

/**
 * Single GNSS observation from one satellite signal.
 * All measurements are raw values from the receiver.
 * Units: metres for pseudorange, cycles for carrier phase, Hz for Doppler.
 */
struct GnssObservation {
    GnssConstellation constellation;
    uint8_t satellite_id;      // Satellite PRN/slot number
    uint8_t signal_id;         // Signal identifier (UBX sigId)
    int8_t frequency_id;       // Frequency channel (GLONASS FDMA, -7 to +6)
    
    // Raw measurements
    double pseudorange_m;      // Pseudorange in metres
    double carrier_phase_cycles; // Carrier phase in cycles
    double doppler_hz;         // Doppler frequency in Hz
    double cn0_db_hz;          // Carrier-to-noise density in dB-Hz
    
    // Measurement quality indicators
    uint16_t lock_time_raw;    // Carrier lock time (receiver-specific encoding)
    uint8_t pr_stdev;          // Pseudorange standard deviation indicator
    uint8_t cp_stdev;          // Carrier phase standard deviation indicator
    uint8_t do_stdev;          // Doppler standard deviation indicator
    
    // Validity flags
    bool pseudorange_valid;
    bool carrier_phase_valid;
    bool doppler_valid;
    
    // Tracking status flags
    bool half_cycle_valid;
    bool half_cycle_subtracted;
    bool carrier_phase_slip;
    
    GnssObservation();
};

/**
 * Collection of observations from all satellites at a single epoch.
 */
struct ObservationEpoch {
    int gps_week;              // GPS week number
    double time_of_week_s;     // Time of week in seconds
    uint8_t leap_seconds;      // GPS-UTC leap seconds (if known)
    bool leap_seconds_valid;
    
    std::vector<GnssObservation> observations;
    
    ObservationEpoch();
    
    // Helper to count observations by constellation
    size_t count_by_constellation(GnssConstellation constellation) const;
};

/**
 * Navigation message subframe data.
 * Stores raw navigation words without decoding ephemeris parameters.
 */
struct NavigationMessage {
    GnssConstellation constellation;
    uint8_t satellite_id;
    uint8_t signal_id;
    int8_t frequency_id;       // For GLONASS
    
    uint8_t num_words;         // Number of 32-bit words
    std::vector<uint32_t> words; // Raw navigation words
    
    // Version and status fields from UBX-RXM-SFRBX
    uint8_t version;
    uint8_t reserved1;
    
    NavigationMessage();
};

/**
 * Receiver-computed PVT solution (from UBX-NAV-PVT).
 * This is the receiver's own position/velocity/time solution
 * used for comparison and validation purposes only.
 */
struct ReceiverPVT {
    // Time
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint32_t nanosecond;
    bool time_valid;
    
    // Position (WGS84)
    double longitude_deg;
    double latitude_deg;
    double height_m;           // Height above ellipsoid
    double height_msl_m;       // Height above mean sea level
    
    // Position accuracy
    double horizontal_accuracy_m;
    double vertical_accuracy_m;
    
    // Velocity (NED frame)
    double vel_north_m_s;
    double vel_east_m_s;
    double vel_down_m_s;
    double ground_speed_m_s;
    double heading_deg;
    double speed_accuracy_m_s;
    double heading_accuracy_deg;
    
    // Fix information
    uint8_t fix_type;          // 0=no fix, 1=dead reckoning, 2=2D, 3=3D, 4=GNSS+DR, 5=time only
    uint8_t num_satellites;
    uint16_t pdop;             // Position DOP (scaled by 0.01)
    
    bool gnss_fix_ok;
    bool diff_soln;            // Differential corrections applied
    bool carr_soln;            // 0=no, 1=float, 2=fixed
    
    ReceiverPVT();
};

} // namespace gnss

#endif // GNSS_OBSERVATION_HPP
