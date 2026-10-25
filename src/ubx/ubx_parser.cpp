#include "gnss/ubx_parser.hpp"
#include <fstream>
#include <cstring>

namespace gnss {

void ParserStatistics::reset() {
    *this = ParserStatistics{};
}

UbxParser::UbxParser()
    : state_(ParseState::SYNC1)
    , bytes_received_(0)
    , checksum_a_(0)
    , checksum_b_(0)
{
    message_buffer_.reserve(ubx::MAX_MESSAGE_SIZE);
}

UbxParser::~UbxParser() = default;

void UbxParser::set_observation_callback(ObservationCallback callback) {
    obs_callback_ = std::move(callback);
}

void UbxParser::set_navigation_callback(NavigationCallback callback) {
    nav_callback_ = std::move(callback);
}

void UbxParser::set_pvt_callback(PvtCallback callback) {
    pvt_callback_ = std::move(callback);
}

bool UbxParser::parse_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read and parse file in chunks
    constexpr size_t CHUNK_SIZE = 8192;
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    
    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE);
        size_t bytes_read = file.gcount();
        
        if (bytes_read > 0) {
            parse_buffer(buffer.data(), bytes_read);
        }
    }
    
    return true;
}

size_t UbxParser::parse_buffer(const uint8_t* data, size_t length) {
    size_t initial_count = stats_.messages_parsed;
    
    for (size_t i = 0; i < length; ++i) {
        process_byte(data[i]);
    }
    
    return stats_.messages_parsed - initial_count;
}

const ParserStatistics& UbxParser::get_statistics() const {
    return stats_;
}

void UbxParser::reset() {
    stats_.reset();
    reset_parser_state();
}

void UbxParser::process_byte(uint8_t byte) {
    stats_.total_bytes_read++;
    
    switch (state_) {
        case ParseState::SYNC1:
            if (byte == ubx::SYNC1) {
                message_buffer_.clear();
                message_buffer_.push_back(byte);
                state_ = ParseState::SYNC2;
            }
            break;
            
        case ParseState::SYNC2:
            message_buffer_.push_back(byte);
            if (byte == ubx::SYNC2) {
                state_ = ParseState::CLASS;
            } else {
                stats_.sync_errors++;
                reset_parser_state();
            }
            break;
            
        case ParseState::CLASS:
            message_buffer_.push_back(byte);
            current_header_.msg_class = byte;
            checksum_a_ = byte;
            checksum_b_ = byte;
            state_ = ParseState::ID;
            break;
            
        case ParseState::ID:
            message_buffer_.push_back(byte);
            current_header_.msg_id = byte;
            checksum_a_ += byte;
            checksum_b_ += checksum_a_;
            state_ = ParseState::LENGTH_LSB;
            break;
            
        case ParseState::LENGTH_LSB:
            message_buffer_.push_back(byte);
            current_header_.payload_length = byte;
            checksum_a_ += byte;
            checksum_b_ += checksum_a_;
            state_ = ParseState::LENGTH_MSB;
            break;
            
        case ParseState::LENGTH_MSB:
            message_buffer_.push_back(byte);
            current_header_.payload_length |= (static_cast<uint16_t>(byte) << 8);
            checksum_a_ += byte;
            checksum_b_ += checksum_a_;
            
            // Sanity check on payload length
            if (current_header_.payload_length > ubx::MAX_MESSAGE_SIZE) {
                stats_.malformed_messages++;
                reset_parser_state();
            } else if (current_header_.payload_length == 0) {
                // No payload, go directly to checksum
                state_ = ParseState::CHECKSUM_A;
            } else {
                bytes_received_ = 0;
                state_ = ParseState::PAYLOAD;
            }
            break;
            
        case ParseState::PAYLOAD:
            message_buffer_.push_back(byte);
            checksum_a_ += byte;
            checksum_b_ += checksum_a_;
            bytes_received_++;
            
            if (bytes_received_ >= current_header_.payload_length) {
                state_ = ParseState::CHECKSUM_A;
            }
            break;
            
        case ParseState::CHECKSUM_A:
            message_buffer_.push_back(byte);
            if (byte == checksum_a_) {
                state_ = ParseState::CHECKSUM_B;
            } else {
                stats_.checksum_failures++;
                reset_parser_state();
            }
            break;
            
        case ParseState::CHECKSUM_B:
            message_buffer_.push_back(byte);
            if (byte == checksum_b_) {
                // Valid message - extract payload and process
                UbxMessage message;
                message.header = current_header_;
                message.checksum_valid = true;
                
                // Extract payload (skip sync bytes, header, and checksums)
                size_t payload_start = 6; // After B5 62 CLASS ID LEN_L LEN_H
                size_t payload_end = payload_start + current_header_.payload_length;
                message.payload.assign(
                    message_buffer_.begin() + payload_start,
                    message_buffer_.begin() + payload_end
                );
                
                stats_.messages_parsed++;
                process_message(message);
            } else {
                stats_.checksum_failures++;
            }
            reset_parser_state();
            break;
    }
}

void UbxParser::process_message(const UbxMessage& message) {
    // Decode based on message type
    if (message.header.msg_class == ubx::CLASS_RXM &&
        message.header.msg_id == ubx::ID_RXM_RAWX) {
        stats_.rawx_count++;
        auto epoch = RawxDecoder::decode(message.payload);
        if (epoch && obs_callback_) {
            obs_callback_(*epoch);
        }
    } else if (message.header.msg_class == ubx::CLASS_RXM &&
               message.header.msg_id == ubx::ID_RXM_SFRBX) {
        stats_.sfrbx_count++;
        auto nav = SfrbxDecoder::decode(message.payload);
        if (nav && nav_callback_) {
            nav_callback_(*nav);
        }
    } else if (message.header.msg_class == ubx::CLASS_NAV &&
               message.header.msg_id == ubx::ID_NAV_PVT) {
        stats_.nav_pvt_count++;
        auto pvt = NavPvtDecoder::decode(message.payload);
        if (pvt && pvt_callback_) {
            pvt_callback_(*pvt);
        }
    } else {
        stats_.other_message_count++;
    }
}

void UbxParser::reset_parser_state() {
    state_ = ParseState::SYNC1;
    message_buffer_.clear();
    bytes_received_ = 0;
    checksum_a_ = 0;
    checksum_b_ = 0;
    current_header_ = UbxMessageHeader{};
}

bool UbxParser::validate_checksum(const uint8_t* message, size_t length) {
    if (length < 8) { // Minimum: sync(2) + class(1) + id(1) + len(2) + checksum(2)
        return false;
    }
    
    uint8_t ck_a = 0;
    uint8_t ck_b = 0;
    
    // Calculate checksum over class, id, length, and payload
    for (size_t i = 2; i < length - 2; ++i) {
        ck_a += message[i];
        ck_b += ck_a;
    }
    
    return (ck_a == message[length - 2]) && (ck_b == message[length - 1]);
}

std::pair<uint8_t, uint8_t> UbxParser::calculate_checksum(
    const uint8_t* message, size_t length)
{
    uint8_t ck_a = 0;
    uint8_t ck_b = 0;
    
    // Calculate checksum over class, id, length, and payload (skip sync bytes)
    for (size_t i = 2; i < length; ++i) {
        ck_a += message[i];
        ck_b += ck_a;
    }
    
    return {ck_a, ck_b};
}

} // namespace gnss
