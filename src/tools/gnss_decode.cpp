#include "gnss/ubx_parser.hpp"
#include "gnss/signal_registry.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <cstring>
#include <algorithm>

using namespace gnss;

/**
 * GNSS Data Decoder - Print all decoded observations to console
 * 
 * This tool decodes UBX files and prints human-readable output
 * for studying GNSS measurements.
 */

void print_header() {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                    GNSS RAW DATA DECODER                                       \n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
}

void print_observation_epoch(const ObservationEpoch& epoch, size_t epoch_number) {
    std::cout << "\n";
    std::cout << "┌─────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ EPOCH #" << std::setw(5) << epoch_number << "                                                               │\n";
    std::cout << "├─────────────────────────────────────────────────────────────────────────────┤\n";
    std::cout << "│ GPS Week:        " << std::setw(6) << epoch.gps_week << std::string(51, ' ') << "│\n";
    std::cout << "│ Time of Week:    " << std::fixed << std::setprecision(3) << std::setw(12) << epoch.time_of_week_s << " seconds" << std::string(37, ' ') << "│\n";
    
    if (epoch.leap_seconds_valid) {
        std::cout << "│ Leap Seconds:    " << std::setw(2) << (int)epoch.leap_seconds << " (GPS-UTC offset)" << std::string(37, ' ') << "│\n";
    }
    
    std::cout << "│ Observations:    " << std::setw(3) << epoch.observations.size() << " measurements" << std::string(47, ' ') << "│\n";
    std::cout << "└─────────────────────────────────────────────────────────────────────────────┘\n";
    
    if (epoch.observations.empty()) {
        std::cout << "  (No observations in this epoch)\n";
        return;
    }
    
    // Group by constellation
    std::map<GnssConstellation, std::vector<const GnssObservation*>> by_constellation;
    for (const auto& obs : epoch.observations) {
        by_constellation[obs.constellation].push_back(&obs);
    }
    
    // Print each constellation
    for (const auto& [constellation, obs_list] : by_constellation) {
        std::cout << "\n";
        std::cout << "  ┌─ " << constellation_to_string(constellation) << " (" << obs_list.size() << " observations) " 
                  << std::string(70 - strlen(constellation_to_string(constellation)), '─') << "┐\n";
        
        for (const auto* obs : obs_list) {
            std::cout << "  │\n";
            std::cout << "  │ Satellite:  " << constellation_to_string(obs->constellation) 
                      << " SV " << std::setw(2) << (int)obs->satellite_id;
            
            // Get signal information
            auto signal_info = SignalRegistry::get_signal_info(obs->constellation, obs->signal_id);
            if (signal_info) {
                std::cout << "  Signal: " << signal_info->signal_name;
            } else {
                std::cout << "  Signal ID: " << (int)obs->signal_id;
            }
            std::cout << "\n";
            
            // Frequency information
            auto freq = SignalRegistry::get_carrier_frequency(obs->constellation, obs->signal_id, obs->frequency_id);
            if (freq) {
                std::cout << "  │   Frequency:    " << std::fixed << std::setprecision(3) << (*freq / 1e6) << " MHz";
                
                // GLONASS channel info
                if (obs->constellation == GnssConstellation::GLONASS) {
                    std::cout << " (Channel " << (int)obs->frequency_id << ")";
                }
                std::cout << "\n";
                
                auto wavelength = SignalRegistry::get_wavelength(obs->constellation, obs->signal_id, obs->frequency_id);
                if (wavelength) {
                    std::cout << "  │   Wavelength:   " << std::fixed << std::setprecision(4) << *wavelength << " metres\n";
                }
            }
            
            std::cout << "  │\n";
            
            // Pseudorange
            std::cout << "  │   Pseudorange:  ";
            if (obs->pseudorange_valid) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(14) << obs->pseudorange_m << " metres [VALID]";
                std::cout << "  (σ=" << (int)obs->pr_stdev << ")";
            } else {
                std::cout << "NOT AVAILABLE";
            }
            std::cout << "\n";
            
            // Carrier Phase
            std::cout << "  │   Carrier Phase:";
            if (obs->carrier_phase_valid) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(14) << obs->carrier_phase_cycles << " cycles  [VALID]";
                std::cout << "  (σ=" << (int)obs->cp_stdev << ")";
                
                // Convert to metres if wavelength available
                auto wavelength = SignalRegistry::get_wavelength(obs->constellation, obs->signal_id, obs->frequency_id);
                if (wavelength) {
                    double phase_metres = obs->carrier_phase_cycles * (*wavelength);
                    std::cout << "\n  │                 " << std::fixed << std::setprecision(3) 
                              << std::setw(14) << phase_metres << " metres  (converted)";
                }
                
                // Half-cycle flags
                if (!obs->half_cycle_valid) {
                    std::cout << "\n  │                 [WARNING: Half-cycle ambiguity]";
                }
                if (obs->half_cycle_subtracted) {
                    std::cout << "\n  │                 [Half-cycle subtracted]";
                }
            } else {
                std::cout << " NOT AVAILABLE";
            }
            std::cout << "\n";
            
            // Doppler
            std::cout << "  │   Doppler:      ";
            if (obs->doppler_valid) {
                std::cout << std::fixed << std::setprecision(3) << std::setw(14) << obs->doppler_hz << " Hz      [VALID]";
                std::cout << "  (σ=" << (int)obs->do_stdev << ")";
            } else {
                std::cout << "NOT AVAILABLE";
            }
            std::cout << "\n";
            
            // Signal Quality
            std::cout << "  │\n";
            std::cout << "  │   Signal Quality:\n";
            std::cout << "  │     C/N0:          " << std::fixed << std::setprecision(1) 
                      << std::setw(5) << obs->cn0_db_hz << " dB-Hz\n";
            std::cout << "  │     Lock Time:     " << std::setw(6) << obs->lock_time_raw << " ms (raw)\n";
        }
        
        std::cout << "  │\n";
        std::cout << "  └" << std::string(77, '-') << "┘\n";
    }
}

void print_navigation_message(const NavigationMessage& nav, size_t msg_number) {
    std::cout << "\n";
    std::cout << "┌─────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ NAVIGATION MESSAGE #" << std::setw(5) << msg_number << "                                                  │\n";
    std::cout << "├─────────────────────────────────────────────────────────────────────────────┤\n";
    std::cout << "│ Constellation:   " << std::setw(10) << std::left << constellation_to_string(nav.constellation) 
              << std::right << std::string(51, ' ') << "│\n";
    std::cout << "│ Satellite:       SV " << std::setw(2) << (int)nav.satellite_id << std::string(56, ' ') << "│\n";
    std::cout << "│ Signal ID:       " << std::setw(3) << (int)nav.signal_id << std::string(58, ' ') << "│\n";
    
    if (nav.constellation == GnssConstellation::GLONASS) {
        std::cout << "│ Frequency Ch:    " << std::setw(3) << (int)nav.frequency_id << std::string(58, ' ') << "│\n";
    }
    
    std::cout << "│ Data Words:      " << std::setw(3) << (int)nav.num_words << " words (32-bit each)" << std::string(42, ' ') << "│\n";
    std::cout << "├─────────────────────────────────────────────────────────────────────────────┤\n";
    std::cout << "│ Raw Navigation Data (Hex):                                                  │\n";
    
    // Print words in rows of 4
    for (size_t i = 0; i < nav.words.size(); i += 4) {
        std::cout << "│  ";
        for (size_t j = i; j < std::min(i + 4, nav.words.size()); ++j) {
            std::cout << std::hex << std::setw(8) << std::setfill('0') << nav.words[j] << " ";
        }
        std::cout << std::dec << std::setfill(' ') << std::string((4 - std::min(4ul, nav.words.size() - i)) * 9 + 40, ' ') << "│\n";
    }
    
    std::cout << "└─────────────────────────────────────────────────────────────────────────────┘\n";
    std::cout << "  NOTE: Ephemeris decoding not implemented in Phase 1.\n";
    std::cout << "        These are raw subframe words from the satellite broadcast.\n";
}

void print_pvt_solution(const ReceiverPVT& pvt, size_t pvt_number) {
    std::cout << "\n";
    std::cout << "┌─────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ RECEIVER PVT SOLUTION #" << std::setw(5) << pvt_number << "                                              │\n";
    std::cout << "├─────────────────────────────────────────────────────────────────────────────┤\n";
    
    // Time
    std::cout << "│ Time (UTC):      " << std::setw(4) << pvt.year << "-" 
              << std::setw(2) << std::setfill('0') << (int)pvt.month << "-" 
              << std::setw(2) << (int)pvt.day << " "
              << std::setw(2) << (int)pvt.hour << ":"
              << std::setw(2) << (int)pvt.minute << ":"
              << std::setw(2) << (int)pvt.second << std::setfill(' ');
    if (pvt.time_valid) {
        std::cout << " [VALID]";
    }
    std::cout << std::string(20, ' ') << "│\n";
    
    std::cout << "│\n";
    
    // Position
    std::cout << "│ Position (WGS84):\n";
    std::cout << "│   Latitude:      " << std::fixed << std::setprecision(9) << std::setw(13) << pvt.latitude_deg << " degrees\n";
    std::cout << "│   Longitude:     " << std::fixed << std::setprecision(9) << std::setw(13) << pvt.longitude_deg << " degrees\n";
    std::cout << "│   Height (HAE):  " << std::fixed << std::setprecision(3) << std::setw(10) << pvt.height_m << " metres\n";
    std::cout << "│   Height (MSL):  " << std::fixed << std::setprecision(3) << std::setw(10) << pvt.height_msl_m << " metres\n";
    std::cout << "│\n";
    
    // Accuracy
    std::cout << "│ Position Accuracy:\n";
    std::cout << "│   Horizontal:    " << std::fixed << std::setprecision(3) << std::setw(10) << pvt.horizontal_accuracy_m << " metres\n";
    std::cout << "│   Vertical:      " << std::fixed << std::setprecision(3) << std::setw(10) << pvt.vertical_accuracy_m << " metres\n";
    std::cout << "│\n";
    
    // Velocity
    std::cout << "│ Velocity (NED):\n";
    std::cout << "│   North:         " << std::fixed << std::setprecision(3) << std::setw(10) << pvt.vel_north_m_s << " m/s\n";
    std::cout << "│   East:          " << std::fixed << std::setprecision(3) << std::setw(10) << pvt.vel_east_m_s << " m/s\n";
    std::cout << "│   Down:          " << std::fixed << std::setprecision(3) << std::setw(10) << pvt.vel_down_m_s << " m/s\n";
    std::cout << "│   Ground Speed:  " << std::fixed << std::setprecision(3) << std::setw(10) << pvt.ground_speed_m_s << " m/s\n";
    std::cout << "│   Heading:       " << std::fixed << std::setprecision(2) << std::setw(10) << pvt.heading_deg << " degrees\n";
    std::cout << "│\n";
    
    // Fix information
    std::cout << "│ Fix Information:\n";
    std::cout << "│   Fix Type:      ";
    switch (pvt.fix_type) {
        case 0: std::cout << "No Fix"; break;
        case 1: std::cout << "Dead Reckoning"; break;
        case 2: std::cout << "2D Fix"; break;
        case 3: std::cout << "3D Fix"; break;
        case 4: std::cout << "GNSS + DR"; break;
        case 5: std::cout << "Time Only"; break;
        default: std::cout << "Unknown (" << (int)pvt.fix_type << ")";
    }
    std::cout << "\n";
    std::cout << "│   Satellites:    " << std::setw(2) << (int)pvt.num_satellites << "\n";
    std::cout << "│   PDOP:          " << std::fixed << std::setprecision(2) << (pvt.pdop * 0.01) << "\n";
    std::cout << "│   GNSS Fix OK:   " << (pvt.gnss_fix_ok ? "YES" : "NO") << "\n";
    std::cout << "│   Diff Soln:     " << (pvt.diff_soln ? "YES" : "NO") << "\n";
    std::cout << "│   Carrier Soln:  ";
    if (pvt.carr_soln == 0) {
        std::cout << "None";
    } else if (pvt.carr_soln == 1) {
        std::cout << "Float";
    } else if (pvt.carr_soln == 2) {
        std::cout << "Fixed";
    } else {
        std::cout << "Unknown";
    }
    std::cout << "\n";
    
    std::cout << "└─────────────────────────────────────────────────────────────────────────────┘\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "GNSS Data Decoder\n";
        std::cout << "=================\n\n";
        std::cout << "Usage: " << argv[0] << " <ubx_file> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --obs-only       Show only observations (skip navigation messages)\n";
        std::cout << "  --nav-only       Show only navigation messages (skip observations)\n";
        std::cout << "  --max-epochs N   Limit output to first N epochs\n";
        std::cout << "  --epoch N        Show only epoch N\n";
        std::cout << "\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " data/rover.ubx\n";
        std::cout << "  " << argv[0] << " data/rover.ubx --obs-only --max-epochs 5\n";
        std::cout << "  " << argv[0] << " data/rover.ubx --epoch 10\n";
        std::cout << "\n";
        return 1;
    }
    
    std::string filename = argv[1];
    bool show_obs = true;
    bool show_nav = true;
    bool show_pvt = true;
    int max_epochs = -1;
    int show_only_epoch = -1;
    
    // Parse options
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--obs-only") {
            show_nav = false;
            show_pvt = false;
        } else if (arg == "--nav-only") {
            show_obs = false;
            show_pvt = false;
        } else if (arg == "--max-epochs" && i + 1 < argc) {
            max_epochs = std::atoi(argv[++i]);
        } else if (arg == "--epoch" && i + 1 < argc) {
            show_only_epoch = std::atoi(argv[++i]);
        }
    }
    
    // Check file exists
    std::ifstream test_file(filename, std::ios::binary);
    if (!test_file) {
        std::cerr << "Error: Cannot open file: " << filename << "\n";
        return 1;
    }
    test_file.close();
    
    print_header();
    std::cout << "Input File: " << filename << "\n";
    std::cout << "Decoding UBX messages...\n";
    
    // Setup parser
    UbxParser parser;
    size_t epoch_count = 0;
    size_t nav_count = 0;
    size_t pvt_count = 0;
    
    if (show_obs) {
        parser.set_observation_callback([&](const ObservationEpoch& epoch) {
            if (show_only_epoch >= 0 && (int)epoch_count != show_only_epoch) {
                epoch_count++;
                return;
            }
            
            if (max_epochs >= 0 && (int)epoch_count >= max_epochs) {
                return;
            }
            
            print_observation_epoch(epoch, epoch_count);
            epoch_count++;
        });
    }
    
    if (show_nav) {
        parser.set_navigation_callback([&](const NavigationMessage& nav) {
            if (show_only_epoch >= 0) return; // Skip nav messages if filtering by epoch
            print_navigation_message(nav, nav_count);
            nav_count++;
        });
    }
    
    if (show_pvt) {
        parser.set_pvt_callback([&](const ReceiverPVT& pvt) {
            if (show_only_epoch >= 0) return; // Skip PVT if filtering by epoch
            print_pvt_solution(pvt, pvt_count);
            pvt_count++;
        });
    }
    
    // Parse file
    if (!parser.parse_file(filename)) {
        std::cerr << "Error: Failed to parse file\n";
        return 1;
    }
    
    // Summary
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                           DECODING COMPLETE                                    \n";
    std::cout << "================================================================================\n";
    std::cout << "\n";
    
    const auto& stats = parser.get_statistics();
    std::cout << "Parser Statistics:\n";
    std::cout << "  Total bytes read:      " << stats.total_bytes_read << "\n";
    std::cout << "  Messages parsed:       " << stats.messages_parsed << "\n";
    std::cout << "  Observation epochs:    " << epoch_count << "\n";
    std::cout << "  Navigation messages:   " << nav_count << "\n";
    std::cout << "  PVT solutions:         " << pvt_count << "\n";
    std::cout << "\n";
    std::cout << "  Checksum failures:     " << stats.checksum_failures << "\n";
    std::cout << "  Sync errors:           " << stats.sync_errors << "\n";
    std::cout << "  Malformed messages:    " << stats.malformed_messages << "\n";
    std::cout << "\n";
    
    if (stats.checksum_failures == 0 && stats.sync_errors == 0 && stats.malformed_messages == 0) {
        std::cout << "✓ All messages decoded successfully!\n";
    } else {
        std::cout << "⚠ Some messages had errors (this may be normal for noisy data)\n";
    }
    
    std::cout << "\n";
    
    return 0;
}
