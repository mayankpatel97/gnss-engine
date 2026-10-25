#ifndef GNSS_DIAGNOSTICS_HPP
#define GNSS_DIAGNOSTICS_HPP

#include "observation.hpp"
#include "ubx_parser.hpp"
#include <map>
#include <string>

namespace gnss {

/**
 * Summary statistics for parsed UBX data.
 */
struct DataSummary {
    // Time span
    int start_gps_week = 0;
    double start_tow_s = 0.0;
    int end_gps_week = 0;
    double end_tow_s = 0.0;
    double duration_s = 0.0;
    
    // Epoch counts
    size_t total_epochs = 0;
    
    // Observation counts
    size_t total_observations = 0;
    size_t valid_pseudorange = 0;
    size_t valid_carrier_phase = 0;
    size_t valid_doppler = 0;
    
    // By constellation
    std::map<GnssConstellation, size_t> obs_by_constellation;
    std::map<GnssConstellation, size_t> satellites_seen;
    
    // By signal
    struct SignalCount {
        GnssConstellation constellation;
        uint8_t signal_id;
        size_t count;
        std::string signal_name;
    };
    std::vector<SignalCount> signals;
    
    // Navigation messages
    size_t nav_messages_total = 0;
    std::map<GnssConstellation, size_t> nav_by_constellation;
    
    // PVT solutions
    size_t pvt_solutions = 0;
    
    // File information
    std::string filename;
    size_t file_size_bytes = 0;
};

/**
 * Diagnostic analyzer for GNSS data.
 * Collects and summarizes observations, navigation messages, and PVT solutions.
 */
class DiagnosticAnalyzer {
public:
    DiagnosticAnalyzer();
    
    /**
     * Add an observation epoch to the summary.
     */
    void add_epoch(const ObservationEpoch& epoch);
    
    /**
     * Add a navigation message to the summary.
     */
    void add_navigation_message(const NavigationMessage& nav);
    
    /**
     * Add a PVT solution to the summary.
     */
    void add_pvt_solution(const ReceiverPVT& pvt);
    
    /**
     * Get the complete data summary.
     */
    const DataSummary& get_summary() const;
    
    /**
     * Reset the analyzer.
     */
    void reset();
    
    /**
     * Print summary to stdout.
     */
    void print_summary() const;

private:
    DataSummary summary_;
    bool first_epoch_ = true;
    
    void update_time_span(int week, double tow);
};

/**
 * CSV exporter for observations.
 */
class CsvExporter {
public:
    /**
     * Export observations to CSV file.
     * Returns true on success, false on error.
     */
    static bool export_observations(
        const std::string& filename,
        const std::vector<ObservationEpoch>& epochs
    );
    
    /**
     * Export navigation messages to CSV file.
     */
    static bool export_navigation_messages(
        const std::string& filename,
        const std::vector<NavigationMessage>& messages
    );
    
    /**
     * Export PVT solutions to CSV file.
     */
    static bool export_pvt_solutions(
        const std::string& filename,
        const std::vector<ReceiverPVT>& solutions
    );
};

} // namespace gnss

#endif // GNSS_DIAGNOSTICS_HPP
