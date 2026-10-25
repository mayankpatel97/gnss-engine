#include "gnss/ubx_parser.hpp"
#include <cstring>

namespace gnss {

namespace {
    // Helper functions to extract little-endian values
    inline uint32_t extract_u4(const uint8_t* p) {
        uint32_t value;
        std::memcpy(&value, p, sizeof(uint32_t));
        return value;
    }
    
    inline uint8_t extract_u1(const uint8_t* p) {
        return *p;
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

std::optional<NavigationMessage> SfrbxDecoder::decode(const std::vector<uint8_t>& payload) {
    // UBX-RXM-SFRBX minimum header size is 8 bytes
    if (payload.size() < 8) {
        return std::nullopt;
    }
    
    const uint8_t* p = payload.data();
    
    NavigationMessage nav;
    
    // Parse header
    uint8_t gnss_id = extract_u1(p);        // GNSS identifier
    uint8_t sv_id = extract_u1(p + 1);      // Satellite identifier
    uint8_t reserved1 = extract_u1(p + 2);  // Reserved
    uint8_t freq_id = extract_u1(p + 3);    // GLONASS frequency slot
    uint8_t num_words = extract_u1(p + 4);  // Number of data words
    uint8_t chn = extract_u1(p + 5);        // Channel number
    uint8_t version = extract_u1(p + 6);    // Message version
    uint8_t reserved2 = extract_u1(p + 7);  // Reserved
    
    nav.constellation = ubx_gnss_id_to_constellation(gnss_id);
    nav.satellite_id = sv_id;
    nav.num_words = num_words;
    nav.version = version;
    nav.reserved1 = reserved1;
    
    // For GLONASS, store frequency ID
    if (nav.constellation == GnssConstellation::GLONASS) {
        nav.frequency_id = static_cast<int8_t>(freq_id) - 7;
    }
    
    // Signal ID is not explicitly provided in SFRBX, infer from context if needed
    // For now, set to 0 (commonly the primary signal for each constellation)
    nav.signal_id = 0;
    
    // Skip unknown constellations
    if (nav.constellation == GnssConstellation::Unknown) {
        return std::nullopt;
    }
    
    // Calculate expected size
    size_t expected_size = 8 + num_words * 4;
    if (payload.size() < expected_size) {
        return std::nullopt;
    }
    
    // Extract navigation words (32-bit words)
    p = payload.data() + 8;
    for (uint8_t i = 0; i < num_words; ++i) {
        uint32_t word = extract_u4(p);
        nav.words.push_back(word);
        p += 4;
    }
    
    return nav;
}

} // namespace gnss
