#include "gnss/ubx_parser.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace gnss;

void test_sfrbx_decoder_invalid_payload() {
    std::cout << "Testing SFRBX decoder with invalid payload...\n";
    
    // Empty payload
    std::vector<uint8_t> empty_payload;
    auto result = SfrbxDecoder::decode(empty_payload);
    assert(!result.has_value());
    
    // Too small payload
    std::vector<uint8_t> small_payload(5, 0);
    result = SfrbxDecoder::decode(small_payload);
    assert(!result.has_value());
    
    std::cout << "  Invalid payload handling: PASSED\n";
}

void test_sfrbx_decoder_gps_subframe() {
    std::cout << "Testing SFRBX decoder with GPS subframe...\n";
    
    // Create a minimal SFRBX message for GPS
    std::vector<uint8_t> payload(8 + 10 * 4, 0);
    
    payload[0] = 0;  // gnssId = 0 (GPS)
    payload[1] = 15; // svId = 15
    payload[2] = 0;  // reserved
    payload[3] = 0;  // freqId (not used for GPS)
    payload[4] = 10; // numWords = 10
    payload[5] = 0;  // chn
    payload[6] = 1;  // version
    payload[7] = 0;  // reserved
    
    // Add 10 dummy navigation words
    for (int i = 0; i < 10; ++i) {
        uint32_t word = 0x12345678 + i;
        std::memcpy(payload.data() + 8 + i * 4, &word, sizeof(uint32_t));
    }
    
    auto result = SfrbxDecoder::decode(payload);
    assert(result.has_value());
    assert(result->constellation == GnssConstellation::GPS);
    assert(result->satellite_id == 15);
    assert(result->num_words == 10);
    assert(result->words.size() == 10);
    
    std::cout << "  GPS subframe decoding: PASSED\n";
}

void test_sfrbx_decoder_unknown_constellation() {
    std::cout << "Testing SFRBX decoder with unknown constellation...\n";
    
    std::vector<uint8_t> payload(8, 0);
    
    payload[0] = 255; // gnssId = 255 (unknown)
    payload[1] = 1;   // svId
    payload[4] = 0;   // numWords = 0
    
    auto result = SfrbxDecoder::decode(payload);
    // Should reject unknown constellations
    assert(!result.has_value());
    
    std::cout << "  Unknown constellation rejection: PASSED\n";
}

int main() {
    std::cout << "=== SFRBX Decoder Unit Tests ===\n\n";
    
    try {
        test_sfrbx_decoder_invalid_payload();
        test_sfrbx_decoder_gps_subframe();
        test_sfrbx_decoder_unknown_constellation();
        
        std::cout << "\nAll tests PASSED!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test FAILED with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Test FAILED with unknown exception\n";
        return 1;
    }
}
