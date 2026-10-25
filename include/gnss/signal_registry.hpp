#ifndef GNSS_SIGNAL_REGISTRY_HPP
#define GNSS_SIGNAL_REGISTRY_HPP

#include "observation.hpp"
#include <optional>

namespace gnss {

/**
 * Signal information for a specific GNSS signal.
 */
struct SignalInfo {
    GnssConstellation constellation;
    uint8_t signal_id;
    const char* signal_name;
    double carrier_frequency_hz; // Nominal frequency (for GLONASS, base frequency)
    double wavelength_m;         // Nominal wavelength (for GLONASS, base wavelength)
    const char* code_name;       // Signal code type (e.g., "L1C/A", "E1-C")
};

/**
 * Registry for GNSS signal identification and properties.
 * Provides mapping from (constellation, signal_id) to signal properties.
 */
class SignalRegistry {
public:
    /**
     * Get signal information for a given constellation and signal ID.
     * Returns nullopt if the signal is not supported or unknown.
     */
    static std::optional<SignalInfo> get_signal_info(
        GnssConstellation constellation,
        uint8_t signal_id
    );
    
    /**
     * Get carrier frequency for a specific signal.
     * For GLONASS FDMA signals, the frequency_id parameter is required.
     * Returns nullopt if the signal is unknown or frequency cannot be determined.
     */
    static std::optional<double> get_carrier_frequency(
        GnssConstellation constellation,
        uint8_t signal_id,
        int8_t frequency_id = 0
    );
    
    /**
     * Get wavelength for a specific signal.
     * For GLONASS FDMA signals, the frequency_id parameter is required.
     * Returns nullopt if the signal is unknown or wavelength cannot be determined.
     */
    static std::optional<double> get_wavelength(
        GnssConstellation constellation,
        uint8_t signal_id,
        int8_t frequency_id = 0
    );
    
    /**
     * Get signal name string.
     * Returns "UNKNOWN" if signal is not recognized.
     */
    static const char* get_signal_name(
        GnssConstellation constellation,
        uint8_t signal_id
    );
    
    /**
     * Check if a signal is supported by the registry.
     */
    static bool is_signal_supported(
        GnssConstellation constellation,
        uint8_t signal_id
    );
};

} // namespace gnss

#endif // GNSS_SIGNAL_REGISTRY_HPP
