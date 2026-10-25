#ifndef GNSS_UBX_PARSER_HPP
#define GNSS_UBX_PARSER_HPP

#include "observation.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <optional>

namespace gnss {

/**
 * UBX message class and ID constants.
 */
namespace ubx {
    // Sync bytes
    constexpr uint8_t SYNC1 = 0xB5;
    constexpr uint8_t SYNC2 = 0x62;
    
    // Message classes
    constexpr uint8_t CLASS_RXM = 0x02;
    constexpr uint8_t CLASS_NAV = 0x01;
    
    // Message IDs
    constexpr uint8_t ID_RXM_RAWX = 0x15;
    constexpr uint8_t ID_RXM_SFRBX = 0x13;
    constexpr uint8_t ID_NAV_PVT = 0x07;
    
    // Maximum message size
    constexpr size_t MAX_MESSAGE_SIZE = 8192;
}

/**
 * UBX message header.
 */
struct UbxMessageHeader {
    uint8_t msg_class;
    uint8_t msg_id;
    uint16_t payload_length;
};

/**
 * Parsed UBX message with header and payload.
 */
struct UbxMessage {
    UbxMessageHeader header;
    std::vector<uint8_t> payload;
    bool checksum_valid;
};

/**
 * Parser statistics.
 */
struct ParserStatistics {
    size_t total_bytes_read = 0;
    size_t messages_parsed = 0;
    size_t checksum_failures = 0;
    size_t sync_errors = 0;
    size_t malformed_messages = 0;
    size_t unsupported_messages = 0;
    
    // Message counts by type
    size_t rawx_count = 0;
    size_t sfrbx_count = 0;
    size_t nav_pvt_count = 0;
    size_t other_message_count = 0;
    
    void reset();
};

/**
 * Callback types for decoded messages.
 */
using ObservationCallback = std::function<void(const ObservationEpoch&)>;
using NavigationCallback = std::function<void(const NavigationMessage&)>;
using PvtCallback = std::function<void(const ReceiverPVT&)>;

/**
 * UBX binary parser.
 * 
 * Reads UBX format files, validates frames, and decodes messages.
 * Handles frame synchronization, checksum validation, and error recovery.
 */
class UbxParser {
public:
    UbxParser();
    ~UbxParser();
    
    /**
     * Set callbacks for decoded messages.
     */
    void set_observation_callback(ObservationCallback callback);
    void set_navigation_callback(NavigationCallback callback);
    void set_pvt_callback(PvtCallback callback);
    
    /**
     * Parse a complete UBX file.
     * Returns true if parsing completed (even if some messages had errors).
     * Returns false only on critical file I/O errors.
     */
    bool parse_file(const std::string& filename);
    
    /**
     * Parse UBX data from a memory buffer.
     * Returns number of messages successfully parsed.
     */
    size_t parse_buffer(const uint8_t* data, size_t length);
    
    /**
     * Get parser statistics.
     */
    const ParserStatistics& get_statistics() const;
    
    /**
     * Reset parser state and statistics.
     */
    void reset();
    
    /**
     * Validate UBX checksum.
     * Static utility function.
     */
    static bool validate_checksum(const uint8_t* message, size_t length);
    
    /**
     * Calculate UBX checksum.
     * Returns pair of (CK_A, CK_B) checksum bytes.
     */
    static std::pair<uint8_t, uint8_t> calculate_checksum(
        const uint8_t* message, size_t length);

private:
    // Internal parsing state
    enum class ParseState {
        SYNC1,
        SYNC2,
        CLASS,
        ID,
        LENGTH_LSB,
        LENGTH_MSB,
        PAYLOAD,
        CHECKSUM_A,
        CHECKSUM_B
    };
    
    ParseState state_;
    std::vector<uint8_t> message_buffer_;
    UbxMessageHeader current_header_;
    size_t bytes_received_;
    uint8_t checksum_a_;
    uint8_t checksum_b_;
    
    ParserStatistics stats_;
    
    // Callbacks
    ObservationCallback obs_callback_;
    NavigationCallback nav_callback_;
    PvtCallback pvt_callback_;
    
    // Process incoming byte
    void process_byte(uint8_t byte);
    
    // Process complete message
    void process_message(const UbxMessage& message);
    
    // Reset to initial state (called after message or error)
    void reset_parser_state();
};

/**
 * RAWX decoder - decodes UBX-RXM-RAWX messages.
 */
class RawxDecoder {
public:
    /**
     * Decode a RAWX message payload.
     * Returns nullopt if the message is malformed or cannot be decoded.
     */
    static std::optional<ObservationEpoch> decode(const std::vector<uint8_t>& payload);
};

/**
 * SFRBX decoder - decodes UBX-RXM-SFRBX messages.
 */
class SfrbxDecoder {
public:
    /**
     * Decode a SFRBX message payload.
     * Returns nullopt if the message is malformed or cannot be decoded.
     */
    static std::optional<NavigationMessage> decode(const std::vector<uint8_t>& payload);
};

/**
 * NAV-PVT decoder - decodes UBX-NAV-PVT messages.
 */
class NavPvtDecoder {
public:
    /**
     * Decode a NAV-PVT message payload.
     * Returns nullopt if the message is malformed or cannot be decoded.
     */
    static std::optional<ReceiverPVT> decode(const std::vector<uint8_t>& payload);
};

} // namespace gnss

#endif // GNSS_UBX_PARSER_HPP
