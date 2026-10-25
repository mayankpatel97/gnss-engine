#include "gnss/signal_registry.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace gnss;

void test_gps_l1_signal() {
    std::cout << "Testing GPS L1 C/A signal...\n";
    
    auto info = SignalRegistry::get_signal_info(GnssConstellation::GPS, 0);
    assert(info.has_value());
    assert(info->constellation == GnssConstellation::GPS);
    assert(info->signal_id == 0);
    
    auto freq = SignalRegistry::get_carrier_frequency(GnssConstellation::GPS, 0);
    assert(freq.has_value());
    assert(std::abs(*freq - 1575.42e6) < 1.0); // GPS L1 frequency
    
    auto wavelength = SignalRegistry::get_wavelength(GnssConstellation::GPS, 0);
    assert(wavelength.has_value());
    assert(*wavelength > 0.19 && *wavelength < 0.20); // ~0.19 m
    
    std::cout << "  GPS L1 C/A: PASSED\n";
}

void test_galileo_e1_signal() {
    std::cout << "Testing Galileo E1 signal...\n";
    
    auto info = SignalRegistry::get_signal_info(GnssConstellation::Galileo, 0);
    assert(info.has_value());
    assert(info->constellation == GnssConstellation::Galileo);
    
    auto freq = SignalRegistry::get_carrier_frequency(GnssConstellation::Galileo, 0);
    assert(freq.has_value());
    assert(std::abs(*freq - 1575.42e6) < 1.0); // Galileo E1 frequency
    
    std::cout << "  Galileo E1: PASSED\n";
}

void test_glonass_fdma() {
    std::cout << "Testing GLONASS FDMA signals...\n";
    
    // GLONASS G1 with frequency ID 0 (channel 0)
    auto freq0 = SignalRegistry::get_carrier_frequency(GnssConstellation::GLONASS, 0, 0);
    assert(freq0.has_value());
    assert(std::abs(*freq0 - 1602.0e6) < 1000.0); // Base frequency
    
    // GLONASS G1 with frequency ID 1 (channel 1)
    auto freq1 = SignalRegistry::get_carrier_frequency(GnssConstellation::GLONASS, 0, 1);
    assert(freq1.has_value());
    
    // Check that frequency increases with channel
    assert(*freq1 > *freq0);
    assert(std::abs((*freq1 - *freq0) - 0.5625e6) < 1000.0); // Channel spacing
    
    std::cout << "  GLONASS FDMA: PASSED\n";
}

void test_unknown_signal() {
    std::cout << "Testing unknown signal...\n";
    
    auto info = SignalRegistry::get_signal_info(GnssConstellation::GPS, 255);
    assert(!info.has_value());
    
    auto freq = SignalRegistry::get_carrier_frequency(GnssConstellation::GPS, 255);
    assert(!freq.has_value());
    
    const char* name = SignalRegistry::get_signal_name(GnssConstellation::GPS, 255);
    assert(std::string(name) == "UNKNOWN");
    
    bool supported = SignalRegistry::is_signal_supported(GnssConstellation::GPS, 255);
    assert(supported == false);
    
    std::cout << "  Unknown signal handling: PASSED\n";
}

void test_beidou_signal() {
    std::cout << "Testing BeiDou B1I signal...\n";
    
    auto info = SignalRegistry::get_signal_info(GnssConstellation::BeiDou, 0);
    assert(info.has_value());
    assert(info->constellation == GnssConstellation::BeiDou);
    
    auto freq = SignalRegistry::get_carrier_frequency(GnssConstellation::BeiDou, 0);
    assert(freq.has_value());
    assert(std::abs(*freq - 1561.098e6) < 1000.0); // BeiDou B1I frequency
    
    std::cout << "  BeiDou B1I: PASSED\n";
}

int main() {
    std::cout << "=== Signal Registry Unit Tests ===\n\n";
    
    try {
        test_gps_l1_signal();
        test_galileo_e1_signal();
        test_glonass_fdma();
        test_unknown_signal();
        test_beidou_signal();
        
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
