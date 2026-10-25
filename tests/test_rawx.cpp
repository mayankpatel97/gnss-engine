#include "gnss/ubx_parser.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace gnss;

void test_rawx_decoder_invalid_payload() {
    std::cout << "Testing RAWX decoder with invalid payload...\n";
    
    // Empty payload
    std::vector<uint8_t> empty_payload;
    auto result = RawxDecoder::decode(empty_payload);
    assert(!result.has_value());
    
    // Too small payload
    std::vector<uint8_t> small_payload(10, 0);
    result = RawxDecoder::decode(small_payload);
    assert(!result.has_value());
    
    std::cout << "  Invalid payload handling: PASSED\n";
}

void test_rawx_decoder_zero_measurements() {
    std::cout << "Testing RAWX decoder with zero measurements...\n";
    
    // Create a minimal valid RAWX header with 0 measurements
    std::vector<uint8_t> payload(16, 0);
    
    // Set rcvTow to 100000.0 (8 bytes double)
    double tow = 100000.0;
    std::memcpy(payload.data(), &tow, sizeof(double));
    
    // Set week to 2200 (2 bytes uint16)
    uint16_t week = 2200;
    std::memcpy(payload.data() + 8, &week, sizeof(uint16_t));
    
    // leap_s = 18
    payload[10] = 18;
    
    // num_meas = 0
    payload[11] = 0;
    
    auto result = RawxDecoder::decode(payload);
    assert(result.has_value());
    assert(result->gps_week == 2200);
    assert(result->time_of_week_s == 100000.0);
    assert(result->observations.empty());
    
    std::cout << "  Zero measurements: PASSED\n";
}

void test_rawx_decoder_week_zero() {
    std::cout << "Testing RAWX decoder with week 0 (invalid)...\n";
    
    std::vector<uint8_t> payload(16, 0);
    
    double tow = 100000.0;
    std::memcpy(payload.data(), &tow, sizeof(double));
    
    // week = 0 (invalid)
    uint16_t week = 0;
    std::memcpy(payload.data() + 8, &week, sizeof(uint16_t));
    
    payload[10] = 18;
    payload[11] = 0;
    
    auto result = RawxDecoder::decode(payload);
    // Should reject week 0
    assert(!result.has_value());
    
    std::cout << "  Week 0 rejection: PASSED\n";
}

int main() {
    std::cout << "=== RAWX Decoder Unit Tests ===\n\n";
    
    try {
        test_rawx_decoder_invalid_payload();
        test_rawx_decoder_zero_measurements();
        test_rawx_decoder_week_zero();
        
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
