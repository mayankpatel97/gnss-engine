#include "gnss/signal_registry.hpp"
#include <array>
#include <cmath>

namespace gnss {

// Physical constants
constexpr double SPEED_OF_LIGHT = 299792458.0; // m/s

// GPS L1/L2/L5 frequencies
constexpr double FREQ_GPS_L1 = 1575.42e6;  // Hz
constexpr double FREQ_GPS_L2 = 1227.60e6;  // Hz
constexpr double FREQ_GPS_L5 = 1176.45e6;  // Hz

// Galileo E1/E5a/E5b/E6 frequencies
constexpr double FREQ_GAL_E1 = 1575.42e6;  // Hz
constexpr double FREQ_GAL_E5a = 1176.45e6; // Hz
constexpr double FREQ_GAL_E5b = 1207.14e6; // Hz
constexpr double FREQ_GAL_E6 = 1278.75e6;  // Hz
constexpr double FREQ_GAL_E5 = 1191.795e6; // Hz (E5a+E5b)

// BeiDou B1/B2/B3 frequencies
constexpr double FREQ_BDS_B1I = 1561.098e6; // Hz
constexpr double FREQ_BDS_B2I = 1207.14e6;  // Hz
constexpr double FREQ_BDS_B3I = 1268.52e6;  // Hz

// GLONASS G1/G2 base frequencies and channel spacing
constexpr double FREQ_GLO_G1_BASE = 1602.0e6;      // Hz
constexpr double FREQ_GLO_G1_CHANNEL = 0.5625e6;   // Hz per channel
constexpr double FREQ_GLO_G2_BASE = 1246.0e6;      // Hz
constexpr double FREQ_GLO_G2_CHANNEL = 0.4375e6;   // Hz per channel

// QZSS frequencies (same as GPS)
constexpr double FREQ_QZSS_L1 = FREQ_GPS_L1;
constexpr double FREQ_QZSS_L2 = FREQ_GPS_L2;
constexpr double FREQ_QZSS_L5 = FREQ_GPS_L5;

// SBAS L1/L5 frequencies
constexpr double FREQ_SBAS_L1 = 1575.42e6;  // Hz
constexpr double FREQ_SBAS_L5 = 1176.45e6;  // Hz

/**
 * Signal database based on u-blox ZED-F9P interface description.
 * sigId values are from u-blox protocol specification.
 */
struct SignalDatabaseEntry {
    GnssConstellation constellation;
    uint8_t signal_id;
    const char* signal_name;
    double frequency_hz;
    const char* code_name;
};

// Signal database - covers common signals from ZED-F9P
static const std::array<SignalDatabaseEntry, 17> signal_database = {{
    // GPS
    {GnssConstellation::GPS, 0, "GPS L1C/A", FREQ_GPS_L1, "L1C/A"},
    {GnssConstellation::GPS, 3, "GPS L2 CL", FREQ_GPS_L2, "L2C"},
    {GnssConstellation::GPS, 4, "GPS L2 CM", FREQ_GPS_L2, "L2C"},
    {GnssConstellation::GPS, 6, "GPS L5 I", FREQ_GPS_L5, "L5"},
    {GnssConstellation::GPS, 7, "GPS L5 Q", FREQ_GPS_L5, "L5"},
    
    // SBAS
    {GnssConstellation::SBAS, 0, "SBAS L1C/A", FREQ_SBAS_L1, "L1C/A"},
    
    // Galileo
    {GnssConstellation::Galileo, 0, "GAL E1 C", FREQ_GAL_E1, "E1-C"},
    {GnssConstellation::Galileo, 1, "GAL E1 B", FREQ_GAL_E1, "E1-B"},
    {GnssConstellation::Galileo, 5, "GAL E5a I", FREQ_GAL_E5a, "E5a"},
    {GnssConstellation::Galileo, 6, "GAL E5a Q", FREQ_GAL_E5a, "E5a"},
    {GnssConstellation::Galileo, 7, "GAL E5b I", FREQ_GAL_E5b, "E5b"},
    {GnssConstellation::Galileo, 8, "GAL E5b Q", FREQ_GAL_E5b, "E5b"},
    
    // BeiDou
    {GnssConstellation::BeiDou, 0, "BDS B1I D1", FREQ_BDS_B1I, "B1I"},
    {GnssConstellation::BeiDou, 1, "BDS B1I D2", FREQ_BDS_B1I, "B1I"},
    {GnssConstellation::BeiDou, 2, "BDS B2I D1", FREQ_BDS_B2I, "B2I"},
    {GnssConstellation::BeiDou, 3, "BDS B2I D2", FREQ_BDS_B2I, "B2I"},
    
    // QZSS
    {GnssConstellation::QZSS, 0, "QZSS L1C/A", FREQ_QZSS_L1, "L1C/A"},
}};

std::optional<SignalInfo> SignalRegistry::get_signal_info(
    GnssConstellation constellation,
    uint8_t signal_id)
{
    // Special handling for GLONASS - frequency depends on channel
    if (constellation == GnssConstellation::GLONASS) {
        if (signal_id == 0) {
            return SignalInfo{
                GnssConstellation::GLONASS,
                0,
                "GLO G1 C/A",
                FREQ_GLO_G1_BASE, // Base frequency
                SPEED_OF_LIGHT / FREQ_GLO_G1_BASE, // Base wavelength
                "G1"
            };
        } else if (signal_id == 2) {
            return SignalInfo{
                GnssConstellation::GLONASS,
                2,
                "GLO G2 C/A",
                FREQ_GLO_G2_BASE, // Base frequency
                SPEED_OF_LIGHT / FREQ_GLO_G2_BASE, // Base wavelength
                "G2"
            };
        }
        return std::nullopt;
    }
    
    // Search signal database
    for (const auto& entry : signal_database) {
        if (entry.constellation == constellation && entry.signal_id == signal_id) {
            double wavelength = SPEED_OF_LIGHT / entry.frequency_hz;
            return SignalInfo{
                entry.constellation,
                entry.signal_id,
                entry.signal_name,
                entry.frequency_hz,
                wavelength,
                entry.code_name
            };
        }
    }
    
    return std::nullopt;
}

std::optional<double> SignalRegistry::get_carrier_frequency(
    GnssConstellation constellation,
    uint8_t signal_id,
    int8_t frequency_id)
{
    // GLONASS FDMA requires frequency_id
    if (constellation == GnssConstellation::GLONASS) {
        if (signal_id == 0) {
            // G1 band: frequency_id ranges from -7 to +6
            return FREQ_GLO_G1_BASE + frequency_id * FREQ_GLO_G1_CHANNEL;
        } else if (signal_id == 2) {
            // G2 band: frequency_id ranges from -7 to +6
            return FREQ_GLO_G2_BASE + frequency_id * FREQ_GLO_G2_CHANNEL;
        }
        return std::nullopt;
    }
    
    auto info = get_signal_info(constellation, signal_id);
    if (info) {
        return info->carrier_frequency_hz;
    }
    return std::nullopt;
}

std::optional<double> SignalRegistry::get_wavelength(
    GnssConstellation constellation,
    uint8_t signal_id,
    int8_t frequency_id)
{
    auto freq = get_carrier_frequency(constellation, signal_id, frequency_id);
    if (freq) {
        return SPEED_OF_LIGHT / (*freq);
    }
    return std::nullopt;
}

const char* SignalRegistry::get_signal_name(
    GnssConstellation constellation,
    uint8_t signal_id)
{
    auto info = get_signal_info(constellation, signal_id);
    if (info) {
        return info->signal_name;
    }
    return "UNKNOWN";
}

bool SignalRegistry::is_signal_supported(
    GnssConstellation constellation,
    uint8_t signal_id)
{
    return get_signal_info(constellation, signal_id).has_value();
}

} // namespace gnss
