#include "gnss/ubx_parser.hpp"
#include <cstring>

namespace gnss {

namespace {
    // Helper functions to extract little-endian values
    inline double extract_r8(const uint8_t* p) {
        double value;
        std::memcpy(&value, p, sizeof(double));
        return value;
    }
    
    inline float extract_r4(const uint8_t* p) {
        float value;
        std::memcpy(&value, p, sizeof(float));
        return value;
    }
    
    inline uint32_t extract_u4(const uint8_t* p) {
        uint32_t value;
        std::memcpy(&value, p, sizeof(uint32_t));
        return value;
    }
    
    inline uint16_t extract_u2(const uint8_t* p) {
        uint16_t value;
        std::memcpy(&value, p, sizeof(uint16_t));
        return value;
    }
    
    inline int16_t extract_i2(const uint8_t* p) {
        int16_t value;
        std::memcpy(&value, p, sizeof(int16_t));
        return value;
    }
    
    inline uint8_t extract_u1(const uint8_t* p) {
        return *p;
    }
    
    inline int8_t extract_i1(const uint8_t* p) {
        return static_cast<int8_t>(*p);
    }
    
    // Convert UBX GNSS ID to constellation
    GnssConstellation ubx_gnss_id_to_constellation(uint8_t gnss_id) {
        switch (gnss_id) {
            case 0: return GnssConstellation::GPS;
            case 1: return GnssConstellation::SBAS;
            case 2: return GnssConstellation::Galileo;
            case 3: return GnssConstellation::BeiDou;
            case 4: return GnssConstellation::IMES;
            case 5: return GnssConstellation::QZSS;
            case 6: return GnssConstellation::GLONASS;
            default: return GnssConstellation::Unknown;
        }
    }
}

std::optional<ObservationEpoch> RawxDecoder::decode(const std::vector<uint8_t>& payload) {
    // UBX-RXM-RAWX minimum header size is 16 bytes
    if (payload.size() < 16) {
        return std::nullopt;
    }
    
    const uint8_t* p = payload.data();
    
    ObservationEpoch epoch;
    
    // Parse header
    double rcv_tow = extract_r8(p);          // Measurement time of week (s)
    uint16_t week = extract_u2(p + 8);       // GPS week number
    int8_t leap_s = extract_i1(p + 10);      // GPS-UTC leap seconds
    uint8_t num_meas = extract_u1(p + 11);   // Number of measurements
    uint8_t rec_stat = extract_u1(p + 12);   // Receiver tracking status
    uint8_t version = extract_u1(p + 13);    // Message version
    // bytes 14-15 are reserved
    
    epoch.gps_week = week;
    epoch.time_of_week_s = rcv_tow;
    
    // Leap seconds
    if (leap_s > 0 && leap_s < 128) {
        epoch.leap_seconds = static_cast<uint8_t>(leap_s);
        epoch.leap_seconds_valid = true;
    }
    
    // Check if week is valid (week 0 is suspicious)
    if (week == 0) {
        return std::nullopt;
    }
    
    // Calculate expected payload size
    size_t expected_size = 16 + num_meas * 32;
    if (payload.size() < expected_size) {
        return std::nullopt;
    }
    
    // Parse measurements
    p = payload.data() + 16; // Start of measurement blocks
    
    for (uint8_t i = 0; i < num_meas; ++i) {
        GnssObservation obs;
        
        // Extract measurement block (32 bytes per measurement)
        double pr_mes = extract_r8(p);       // Pseudorange (m)
        double cp_mes = extract_r8(p + 8);   // Carrier phase (cycles)
        float do_mes = extract_r4(p + 16);   // Doppler (Hz)
        uint8_t gnss_id = extract_u1(p + 20);    // GNSS identifier
        uint8_t sv_id = extract_u1(p + 21);      // Satellite identifier
        uint8_t sig_id = extract_u1(p + 22);     // Signal identifier (new in version 1)
        uint8_t freq_id = extract_u1(p + 23);    // Frequency slot + 7 (for GLONASS)
        uint16_t locktime = extract_u2(p + 24);  // Carrier phase locktime (ms)
        uint8_t cno = extract_u1(p + 26);        // C/N0 (dB-Hz)
        uint8_t pr_stdev = extract_u1(p + 27);   // Pseudorange stdev (low 4 bits)
        uint8_t cp_stdev = extract_u1(p + 28);   // Carrier phase stdev (low 4 bits)
        uint8_t do_stdev = extract_u1(p + 29);   // Doppler stdev (low 4 bits)
        uint8_t trk_stat = extract_u1(p + 30);   // Tracking status
        uint8_t reserved = extract_u1(p + 31);
        
        // Populate observation
        obs.constellation = ubx_gnss_id_to_constellation(gnss_id);
        obs.satellite_id = sv_id;
        obs.signal_id = sig_id;
        
        // GLONASS frequency ID: stored as freq_id = channel + 7, so channel = freq_id - 7
        obs.frequency_id = static_cast<int8_t>(freq_id) - 7;
        
        // Measurements
        obs.pseudorange_m = pr_mes;
        obs.carrier_phase_cycles = cp_mes;
        obs.doppler_hz = do_mes;
        obs.cn0_db_hz = static_cast<double>(cno);
        
        // Quality indicators (lower 4 bits contain the actual values)
        obs.pr_stdev = pr_stdev & 0x0F;
        obs.cp_stdev = cp_stdev & 0x0F;
        obs.do_stdev = do_stdev & 0x0F;
        obs.lock_time_raw = locktime;
        
        // Tracking status flags (bit field)
        // Bit 0: PR valid
        // Bit 1: CP valid
        // Bit 2: half cycle valid
        // Bit 3: half cycle subtracted
        obs.pseudorange_valid = (trk_stat & 0x01) != 0;
        obs.carrier_phase_valid = (trk_stat & 0x02) != 0;
        obs.half_cycle_valid = (trk_stat & 0x04) != 0;
        obs.half_cycle_subtracted = (trk_stat & 0x08) != 0;
        
        // Doppler is valid if both PR and CP are being tracked
        obs.doppler_valid = obs.pseudorange_valid || obs.carrier_phase_valid;
        
        // Additional validity checks:
        // - Carrier phase of -0.5 cycles indicates invalid
        // - High CP standard deviation indicates invalid
        constexpr uint8_t CP_STDEV_THRESHOLD = 5;
        if (cp_mes == -0.5 || obs.cp_stdev > CP_STDEV_THRESHOLD) {
            obs.carrier_phase_valid = false;
        }
        
        // Skip observations from unknown constellations
        if (obs.constellation != GnssConstellation::Unknown) {
            epoch.observations.push_back(obs);
        }
        
        // Move to next measurement block
        p += 32;
    }
    
    return epoch;
}

} // namespace gnss
