#include "gnss/ubx_parser.hpp"
#include "gnss/signal_registry.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>

using namespace gnss;

/**
 * GNSS Parser - Extract measurements from UBX and output to table format
 * 
 * This tool parses UBX files and outputs measurements in clean table format
 * to both console and file.
 */

class TableWriter {
public:
    TableWriter(const std::string& filename) {
        if (!filename.empty()) {
            file_.open(filename);
            if (!file_) {
                std::cerr << "Warning: Could not open output file: " << filename << "\n";
            }
        }
    }
    
    void write_header() {
        const std::string header = 
            "┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\n"
            "│                                            GNSS MEASUREMENTS PARSER                                                                │\n"
            "└────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘\n";
        
        write_line(header);
    }
    
    void write_epoch_header(size_t epoch_num, int gps_week, double tow, size_t num_obs) {
        std::ostringstream oss;
        oss << "\n"
            << "═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n"
            << " EPOCH #" << std::setw(5) << epoch_num 
            << "  │  GPS Week: " << std::setw(4) << gps_week
            << "  │  TOW: " << std::fixed << std::setprecision(3) << std::setw(12) << tow << " s"
            << "  │  Observations: " << std::setw(3) << num_obs << "\n"
            << "═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
        
        write_line(oss.str());
    }
    
    void write_table_header() {
        std::ostringstream oss;
        oss << std::left
            << std::setw(10) << "Const"
            << std::setw(5) << "SV"
            << std::setw(8) << "SigID"
            << std::setw(17) << "Pseudorange(m)"
            << std::setw(3) << "V"
            << std::setw(20) << "CarrierPhase(cy)"
            << std::setw(3) << "V"
            << std::setw(12) << "Doppler(Hz)"
            << std::setw(3) << "V"
            << std::setw(9) << "CN0(dBHz)"
            << std::setw(12) << "Signal"
            << "\n";
        
        oss << std::string(140, '-') << "\n";
        
        write_line(oss.str());
    }
    
    void write_observation(const GnssObservation& obs) {
        std::ostringstream oss;
        
        // Constellation and satellite
        oss << std::left << std::setw(10) << constellation_to_string(obs.constellation);
        oss << std::setw(5) << (int)obs.satellite_id;
        oss << std::setw(8) << (int)obs.signal_id;
        
        // Pseudorange
        if (obs.pseudorange_valid) {
            oss << std::fixed << std::setprecision(3) << std::setw(17) << obs.pseudorange_m;
            oss << std::setw(3) << "Y";
        } else {
            oss << std::setw(17) << "N/A";
            oss << std::setw(3) << "N";
        }
        
        // Carrier Phase
        if (obs.carrier_phase_valid) {
            oss << std::fixed << std::setprecision(3) << std::setw(20) << obs.carrier_phase_cycles;
            oss << std::setw(3) << "Y";
        } else {
            oss << std::setw(20) << "N/A";
            oss << std::setw(3) << "N";
        }
        
        // Doppler
        if (obs.doppler_valid) {
            oss << std::fixed << std::setprecision(3) << std::setw(12) << obs.doppler_hz;
            oss << std::setw(3) << "Y";
        } else {
            oss << std::setw(12) << "N/A";
            oss << std::setw(3) << "N";
        }
        
        // CN0
        oss << std::fixed << std::setprecision(1) << std::setw(9) << obs.cn0_db_hz;
        
        // Signal name
        auto signal_info = SignalRegistry::get_signal_info(obs.constellation, obs.signal_id);
        if (signal_info) {
            oss << std::setw(12) << signal_info->signal_name;
        } else {
            oss << std::setw(12) << "UNKNOWN";
        }
        
        oss << "\n";
        
        write_line(oss.str());
    }
    
    void write_summary(size_t total_epochs, size_t total_obs, 
                      size_t valid_pr, size_t valid_cp, size_t valid_do) {
        std::ostringstream oss;
        oss << "\n"
            << "═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n"
            << " SUMMARY\n"
            << "═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n"
            << " Total Epochs:           " << total_epochs << "\n"
            << " Total Observations:     " << total_obs << "\n"
            << " Valid Pseudorange:      " << valid_pr << " (" << (100.0 * valid_pr / total_obs) << "%)\n"
            << " Valid Carrier Phase:    " << valid_cp << " (" << (100.0 * valid_cp / total_obs) << "%)\n"
            << " Valid Doppler:          " << valid_do << " (" << (100.0 * valid_do / total_obs) << "%)\n"
            << "═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════\n";
        
        write_line(oss.str());
    }
    
    ~TableWriter() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
private:
    std::ofstream file_;
    
    void write_line(const std::string& line) {
        // Write to console
        std::cout << line;
        
        // Write to file if open
        if (file_.is_open()) {
            file_ << line;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "GNSS Measurements Parser\n";
        std::cout << "========================\n\n";
        std::cout << "Usage: " << argv[0] << " <ubx_file> [output_file]\n\n";
        std::cout << "Arguments:\n";
        std::cout << "  ubx_file     - Input UBX file to parse\n";
        std::cout << "  output_file  - Optional output file (default: measurements.txt)\n";
        std::cout << "\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " data/rover.ubx\n";
        std::cout << "  " << argv[0] << " data/rover.ubx output.txt\n";
        std::cout << "\n";
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = (argc >= 3) ? argv[2] : "measurements.txt";
    
    // Check input file exists
    std::ifstream test_file(input_file, std::ios::binary);
    if (!test_file) {
        std::cerr << "Error: Cannot open input file: " << input_file << "\n";
        return 1;
    }
    test_file.close();
    
    // Create table writer
    TableWriter writer(output_file);
    
    writer.write_header();
    std::cout << "Parsing: " << input_file << "\n";
    std::cout << "Output:  " << output_file << "\n\n";
    
    // Setup parser
    UbxParser parser;
    size_t epoch_count = 0;
    size_t total_observations = 0;
    size_t valid_pr = 0;
    size_t valid_cp = 0;
    size_t valid_do = 0;
    
    parser.set_observation_callback([&](const ObservationEpoch& epoch) {
        writer.write_epoch_header(epoch_count, epoch.gps_week, epoch.time_of_week_s, 
                                 epoch.observations.size());
        writer.write_table_header();
        
        for (const auto& obs : epoch.observations) {
            writer.write_observation(obs);
            
            total_observations++;
            if (obs.pseudorange_valid) valid_pr++;
            if (obs.carrier_phase_valid) valid_cp++;
            if (obs.doppler_valid) valid_do++;
        }
        
        epoch_count++;
    });
    
    // Parse file
    std::cout << "Processing...\n";
    if (!parser.parse_file(input_file)) {
        std::cerr << "Error: Failed to parse file\n";
        return 1;
    }
    
    // Write summary
    writer.write_summary(epoch_count, total_observations, valid_pr, valid_cp, valid_do);
    
    // Parser statistics
    const auto& stats = parser.get_statistics();
    std::cout << "\nParser Statistics:\n";
    std::cout << "------------------\n";
    std::cout << "Messages parsed:      " << stats.messages_parsed << "\n";
    std::cout << "Checksum failures:    " << stats.checksum_failures << "\n";
    std::cout << "Sync errors:          " << stats.sync_errors << "\n";
    std::cout << "\n";
    
    if (stats.checksum_failures == 0 && stats.sync_errors == 0) {
        std::cout << "✓ Parsing completed successfully!\n";
    } else {
        std::cout << "⚠ Some errors occurred during parsing\n";
    }
    
    std::cout << "\nOutput written to: " << output_file << "\n";
    
    return 0;
}
