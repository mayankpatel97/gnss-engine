#include "gnss/ubx_parser.hpp"
#include <cstring>

namespace gnss {

namespace {
    // Helper functions to extract little-endian values
    inline int32_t extract_i4(const uint8_t* p) {
        int32_t value;
        std::memcpy(&value, p, sizeof(int32_t));
        return value;
    }
    
    inline uint32_t extract_u4(const uint8_t* p) {
        uint32_t value;
        std::memcpy(&value, p, sizeof(uint32_t));
        return value;
    }
    
    inline int16_t extract_i2(const uint8_t* p) {
        int16_t value;
        std::memcpy(&value, p, sizeof(int16_t));
        return value;
    }
    
    inline uint16_t extract_u2(const uint8_t* p) {
        uint16_t value;
        std::memcpy(&value, p, sizeof(uint16_t));
        return value;
    }
    
    inline uint8_t extract_u1(const uint8_t* p) {
        return *p;
    }
    
    inline int8_t extract_i1(const uint8_t* p) {
        return static_cast<int8_t>(*p);
    }
}

std::optional<ReceiverPVT> NavPvtDecoder::decode(const std::vector<uint8_t>& payload) {
    // UBX-NAV-PVT payload size is 92 bytes for most versions
    if (payload.size() < 84) { // Minimum reasonable size
        return std::nullopt;
    }
    
    const uint8_t* p = payload.data();
    
    ReceiverPVT pvt;
    
    // Parse time
    uint32_t iTOW = extract_u4(p);           // GPS time of week (ms)
    uint16_t year = extract_u2(p + 4);       // Year (UTC)
    uint8_t month = extract_u1(p + 6);       // Month (1-12)
    uint8_t day = extract_u1(p + 7);         // Day (1-31)
    uint8_t hour = extract_u1(p + 8);        // Hour (0-23)
    uint8_t min = extract_u1(p + 9);         // Minute (0-59)
    uint8_t sec = extract_u1(p + 10);        // Second (0-60)
    uint8_t valid = extract_u1(p + 11);      // Validity flags
    uint32_t tAcc = extract_u4(p + 12);      // Time accuracy estimate (ns)
    int32_t nano = extract_i4(p + 16);       // Fraction of second (-1e9 to 1e9 ns)
    uint8_t fixType = extract_u1(p + 20);    // Fix type
    uint8_t flags = extract_u1(p + 21);      // Fix status flags
    uint8_t flags2 = extract_u1(p + 22);     // Additional flags
    uint8_t numSV = extract_u1(p + 23);      // Number of satellites used
    
    pvt.year = year;
    pvt.month = month;
    pvt.day = day;
    pvt.hour = hour;
    pvt.minute = min;
    pvt.second = sec;
    pvt.nanosecond = nano >= 0 ? static_cast<uint32_t>(nano) : 0;
    pvt.time_valid = (valid & 0x03) == 0x03; // validDate and validTime
    pvt.fix_type = fixType;
    pvt.num_satellites = numSV;
    
    // Parse position (WGS84)
    int32_t lon = extract_i4(p + 24);        // Longitude (deg * 1e-7)
    int32_t lat = extract_i4(p + 28);        // Latitude (deg * 1e-7)
    int32_t height = extract_i4(p + 32);     // Height above ellipsoid (mm)
    int32_t hMSL = extract_i4(p + 36);       // Height above MSL (mm)
    uint32_t hAcc = extract_u4(p + 40);      // Horizontal accuracy (mm)
    uint32_t vAcc = extract_u4(p + 44);      // Vertical accuracy (mm)
    
    pvt.longitude_deg = lon * 1e-7;
    pvt.latitude_deg = lat * 1e-7;
    pvt.height_m = height * 1e-3;
    pvt.height_msl_m = hMSL * 1e-3;
    pvt.horizontal_accuracy_m = hAcc * 1e-3;
    pvt.vertical_accuracy_m = vAcc * 1e-3;
    
    // Parse velocity (NED frame)
    int32_t velN = extract_i4(p + 48);       // North velocity (mm/s)
    int32_t velE = extract_i4(p + 52);       // East velocity (mm/s)
    int32_t velD = extract_i4(p + 56);       // Down velocity (mm/s)
    int32_t gSpeed = extract_i4(p + 60);     // Ground speed (mm/s)
    int32_t headMot = extract_i4(p + 64);    // Heading of motion (deg * 1e-5)
    uint32_t sAcc = extract_u4(p + 68);      // Speed accuracy (mm/s)
    uint32_t headAcc = extract_u4(p + 72);   // Heading accuracy (deg * 1e-5)
    uint16_t pDOP = extract_u2(p + 76);      // Position DOP (* 0.01)
    
    pvt.vel_north_m_s = velN * 1e-3;
    pvt.vel_east_m_s = velE * 1e-3;
    pvt.vel_down_m_s = velD * 1e-3;
    pvt.ground_speed_m_s = gSpeed * 1e-3;
    pvt.heading_deg = headMot * 1e-5;
    pvt.speed_accuracy_m_s = sAcc * 1e-3;
    pvt.heading_accuracy_deg = headAcc * 1e-5;
    pvt.pdop = pDOP;
    
    // Parse fix flags
    pvt.gnss_fix_ok = (flags & 0x01) != 0;
    pvt.diff_soln = (flags & 0x02) != 0;
    
    // Carrier solution (bits 6-7 of flags)
    uint8_t carrSoln = (flags >> 6) & 0x03;
    pvt.carr_soln = carrSoln;
    
    return pvt;
}

} // namespace gnss
