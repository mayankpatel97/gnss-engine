#include "gnss/ubx_parser.hpp"
#include "gnss/diagnostics.hpp"
#include "gnss/signal_registry.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>

using namespace gnss;

struct AppOptions {
    std::string input_file;
    bool print_epochs = false;
    bool print_epoch_detail = false;
    int selected_epoch = -1;
    bool print_satellite_summary = false;
    bool export_observations_csv = false;
    bool export_navigation_csv = false;
    bool export_pvt_csv = false;
    std::string csv_obs_output;
    std::string csv_nav_output;
    std::string csv_pvt_output;
    bool verbose = false;
};

void print_usage(const char* prog_name) {
    std::cout << "GNSS Data Inspector\n";
    std::cout << "Usage: " << prog_name << " [options] <ubx_file>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -e, --epochs            Print all epoch times\n";
    std::cout << "  -d, --detail N          Print detailed measurements for epoch N\n";
    std::cout << "  -s, --sat-summary       Print satellite/signal summary\n";
    std::cout << "  --csv-obs FILE          Export observations to CSV file\n";
    std::cout << "  --csv-nav FILE          Export navigation messages to CSV file\n";
    std::cout << "  --csv-pvt FILE          Export PVT solutions to CSV file\n";
    std::cout << "  -v, --verbose           Verbose output\n";
    std::cout << "\n";
}

bool parse_arguments(int argc, char* argv[], AppOptions& options) {
    if (argc < 2) {
        return false;
    }
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            return false;
        } else if (arg == "-e" || arg == "--epochs") {
            options.print_epochs = true;
        } else if (arg == "-d" || arg == "--detail") {
            if (i + 1 < argc) {
                options.print_epoch_detail = true;
                options.selected_epoch = std::atoi(argv[++i]);
            }
        } else if (arg == "-s" || arg == "--sat-summary") {
            options.print_satellite_summary = true;
        } else if (arg == "--csv-obs") {
            if (i + 1 < argc) {
                options.export_observations_csv = true;
                options.csv_obs_output = argv[++i];
            }
        } else if (arg == "--csv-nav") {
            if (i + 1 < argc) {
                options.export_navigation_csv = true;
                options.csv_nav_output = argv[++i];
            }
        } else if (arg == "--csv-pvt") {
            if (i + 1 < argc) {
                options.export_pvt_csv = true;
                options.csv_pvt_output = argv[++i];
            }
        } else if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg[0] != '-') {
            options.input_file = arg;
        }
    }
    
    return !options.input_file.empty();
}

class DataCollector {
public:
    std::vector<ObservationEpoch> epochs;
    std::vector<NavigationMessage> nav_messages;
    std::vector<ReceiverPVT> pvt_solutions;
    DiagnosticAnalyzer analyzer;
    
    void on_observation(const ObservationEpoch& epoch) {
        epochs.push_back(epoch);
        analyzer.add_epoch(epoch);
    }
    
    void on_navigation(const NavigationMessage& nav) {
        nav_messages.push_back(nav);
        analyzer.add_navigation_message(nav);
    }
    
    void on_pvt(const ReceiverPVT& pvt) {
        pvt_solutions.push_back(pvt);
        analyzer.add_pvt_solution(pvt);
    }
};

void print_epoch_list(const std::vector<ObservationEpoch>& epochs) {
    std::cout << "\n=== Observation Epochs ===\n";
    std::cout << "Index | GPS Week | TOW (s)     | Num Obs\n";
    std::cout << "------|----------|-------------|--------\n";
    
    for (size_t i = 0; i < epochs.size(); ++i) {
        std::cout << std::setw(5) << i << " | "
                  << std::setw(8) << epochs[i].gps_week << " | "
                  << std::fixed << std::setprecision(3) << std::setw(11) << epochs[i].time_of_week_s << " | "
                  << std::setw(7) << epochs[i].observations.size() << "\n";
    }
}

void print_epoch_detail(const ObservationEpoch& epoch, int index) {
    std::cout << "\n=== Epoch " << index << " Detail ===\n";
    std::cout << "GPS Week: " << epoch.gps_week << "\n";
    std::cout << "Time of Week: " << std::fixed << std::setprecision(6) << epoch.time_of_week_s << " s\n";
    std::cout << "Total Observations: " << epoch.observations.size() << "\n\n";
    
    std::cout << "Const.    | SV | Sig | PR Valid | CP Valid | DO Valid | Pseudorange (m) | Carrier Phase (cy) | Doppler (Hz) | CN0 (dB-Hz)\n";
    std::cout << "----------|----|----|----------|----------|----------|-----------------|-------------------|--------------|------------\n";
    
    for (const auto& obs : epoch.observations) {
        std::cout << std::setw(9) << std::left << constellation_to_string(obs.constellation) << " | "
                  << std::setw(2) << std::right << static_cast<int>(obs.satellite_id) << " | "
                  << std::setw(2) << static_cast<int>(obs.signal_id) << " | "
                  << std::setw(8) << (obs.pseudorange_valid ? "YES" : "NO") << " | "
                  << std::setw(8) << (obs.carrier_phase_valid ? "YES" : "NO") << " | "
                  << std::setw(8) << (obs.doppler_valid ? "YES" : "NO") << " | "
                  << std::fixed << std::setprecision(3) << std::setw(15) << (obs.pseudorange_valid ? obs.pseudorange_m : 0.0) << " | "
                  << std::fixed << std::setprecision(3) << std::setw(17) << (obs.carrier_phase_valid ? obs.carrier_phase_cycles : 0.0) << " | "
                  << std::fixed << std::setprecision(2) << std::setw(12) << (obs.doppler_valid ? obs.doppler_hz : 0.0) << " | "
                  << std::fixed << std::setprecision(1) << std::setw(11) << obs.cn0_db_hz << "\n";
    }
}

void print_satellite_summary(const std::vector<ObservationEpoch>& epochs) {
    std::cout << "\n=== Satellite/Signal Summary ===\n";
    
    // Collect unique satellite/signal combinations
    struct SatSignalKey {
        GnssConstellation constellation;
        uint8_t satellite_id;
        uint8_t signal_id;
        
        bool operator<(const SatSignalKey& other) const {
            if (constellation != other.constellation)
                return constellation < other.constellation;
            if (satellite_id != other.satellite_id)
                return satellite_id < other.satellite_id;
            return signal_id < other.signal_id;
        }
    };
    
    std::map<SatSignalKey, size_t> sat_signal_counts;
    
    for (const auto& epoch : epochs) {
        for (const auto& obs : epoch.observations) {
            SatSignalKey key{obs.constellation, obs.satellite_id, obs.signal_id};
            sat_signal_counts[key]++;
        }
    }
    
    std::cout << "Constellation | SV | Signal ID | Signal Name       | Epochs | Frequency (MHz)\n";
    std::cout << "--------------|----|-----------|--------------------|--------|----------------\n";
    
    for (const auto& [key, count] : sat_signal_counts) {
        auto signal_name = SignalRegistry::get_signal_name(key.constellation, key.signal_id);
        auto freq = SignalRegistry::get_carrier_frequency(key.constellation, key.signal_id, 0);
        
        std::cout << std::setw(13) << std::left << constellation_to_string(key.constellation) << " | "
                  << std::setw(2) << std::right << static_cast<int>(key.satellite_id) << " | "
                  << std::setw(9) << static_cast<int>(key.signal_id) << " | "
                  << std::setw(18) << std::left << signal_name << " | "
                  << std::setw(6) << std::right << count << " | ";
        
        if (freq) {
            std::cout << std::fixed << std::setprecision(3) << (*freq / 1e6);
        } else {
            std::cout << "Unknown";
        }
        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    AppOptions options;
    
    if (!parse_arguments(argc, argv, options)) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Check if file exists
    std::ifstream test_file(options.input_file, std::ios::binary);
    if (!test_file) {
        std::cerr << "Error: Cannot open file: " << options.input_file << "\n";
        return 1;
    }
    
    // Get file size
    test_file.seekg(0, std::ios::end);
    size_t file_size = test_file.tellg();
    test_file.close();
    
    std::cout << "GNSS Data Inspector\n";
    std::cout << "===================\n\n";
    std::cout << "Input file: " << options.input_file << "\n";
    std::cout << "File size: " << file_size << " bytes (" 
              << std::fixed << std::setprecision(2) << (file_size / 1024.0 / 1024.0) << " MB)\n\n";
    
    // Parse the file
    DataCollector collector;
    UbxParser parser;
    
    parser.set_observation_callback([&collector](const ObservationEpoch& epoch) {
        collector.on_observation(epoch);
    });
    
    parser.set_navigation_callback([&collector](const NavigationMessage& nav) {
        collector.on_navigation(nav);
    });
    
    parser.set_pvt_callback([&collector](const ReceiverPVT& pvt) {
        collector.on_pvt(pvt);
    });
    
    std::cout << "Parsing file...\n";
    
    if (!parser.parse_file(options.input_file)) {
        std::cerr << "Error: Failed to parse file\n";
        return 1;
    }
    
    std::cout << "Parsing complete.\n\n";
    
    // Print statistics
    const auto& stats = parser.get_statistics();
    std::cout << "=== Parser Statistics ===\n";
    std::cout << "Total bytes read:       " << stats.total_bytes_read << "\n";
    std::cout << "Messages parsed:        " << stats.messages_parsed << "\n";
    std::cout << "Checksum failures:      " << stats.checksum_failures << "\n";
    std::cout << "Sync errors:            " << stats.sync_errors << "\n";
    std::cout << "Malformed messages:     " << stats.malformed_messages << "\n";
    std::cout << "\n";
    std::cout << "Message type counts:\n";
    std::cout << "  RXM-RAWX:             " << stats.rawx_count << "\n";
    std::cout << "  RXM-SFRBX:            " << stats.sfrbx_count << "\n";
    std::cout << "  NAV-PVT:              " << stats.nav_pvt_count << "\n";
    std::cout << "  Other:                " << stats.other_message_count << "\n";
    std::cout << "\n";
    
    // Print data summary
    collector.analyzer.print_summary();
    
    // Optional outputs
    if (options.print_epochs && !collector.epochs.empty()) {
        print_epoch_list(collector.epochs);
    }
    
    if (options.print_epoch_detail && options.selected_epoch >= 0 &&
        options.selected_epoch < static_cast<int>(collector.epochs.size())) {
        print_epoch_detail(collector.epochs[options.selected_epoch], options.selected_epoch);
    }
    
    if (options.print_satellite_summary && !collector.epochs.empty()) {
        print_satellite_summary(collector.epochs);
    }
    
    // CSV exports
    if (options.export_observations_csv && !collector.epochs.empty()) {
        std::cout << "\nExporting observations to " << options.csv_obs_output << "...\n";
        if (CsvExporter::export_observations(options.csv_obs_output, collector.epochs)) {
            std::cout << "Export successful.\n";
        } else {
            std::cerr << "Export failed.\n";
        }
    }
    
    if (options.export_navigation_csv && !collector.nav_messages.empty()) {
        std::cout << "\nExporting navigation messages to " << options.csv_nav_output << "...\n";
        if (CsvExporter::export_navigation_messages(options.csv_nav_output, collector.nav_messages)) {
            std::cout << "Export successful.\n";
        } else {
            std::cerr << "Export failed.\n";
        }
    }
    
    if (options.export_pvt_csv && !collector.pvt_solutions.empty()) {
        std::cout << "\nExporting PVT solutions to " << options.csv_pvt_output << "...\n";
        if (CsvExporter::export_pvt_solutions(options.csv_pvt_output, collector.pvt_solutions)) {
            std::cout << "Export successful.\n";
        } else {
            std::cerr << "Export failed.\n";
        }
    }
    
    return 0;
}
