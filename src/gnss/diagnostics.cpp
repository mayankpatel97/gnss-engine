#include "gnss/diagnostics.hpp"
#include "gnss/signal_registry.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>

namespace gnss {

DiagnosticAnalyzer::DiagnosticAnalyzer() = default;

void DiagnosticAnalyzer::add_epoch(const ObservationEpoch& epoch) {
    if (first_epoch_) {
        summary_.start_gps_week = epoch.gps_week;
        summary_.start_tow_s = epoch.time_of_week_s;
        first_epoch_ = false;
    }
    
    update_time_span(epoch.gps_week, epoch.time_of_week_s);
    summary_.total_epochs++;
    
    // Count observations
    summary_.total_observations += epoch.observations.size();
    
    // Track satellites seen per constellation
    std::map<GnssConstellation, std::set<uint8_t>> sats_this_epoch;
    
    for (const auto& obs : epoch.observations) {
        // Count valid measurements
        if (obs.pseudorange_valid) {
            summary_.valid_pseudorange++;
        }
        if (obs.carrier_phase_valid) {
            summary_.valid_carrier_phase++;
        }
        if (obs.doppler_valid) {
            summary_.valid_doppler++;
        }
        
        // Count by constellation
        summary_.obs_by_constellation[obs.constellation]++;
        
        // Track unique satellites
        sats_this_epoch[obs.constellation].insert(obs.satellite_id);
        
        // Track signals
        bool signal_found = false;
        for (auto& sig : summary_.signals) {
            if (sig.constellation == obs.constellation && sig.signal_id == obs.signal_id) {
                sig.count++;
                signal_found = true;
                break;
            }
        }
        if (!signal_found) {
            DataSummary::SignalCount sig_count;
            sig_count.constellation = obs.constellation;
            sig_count.signal_id = obs.signal_id;
            sig_count.count = 1;
            sig_count.signal_name = SignalRegistry::get_signal_name(obs.constellation, obs.signal_id);
            summary_.signals.push_back(sig_count);
        }
    }
    
    // Update satellites seen
    for (const auto& [constellation, sats] : sats_this_epoch) {
        if (sats.size() > summary_.satellites_seen[constellation]) {
            summary_.satellites_seen[constellation] = sats.size();
        }
    }
}

void DiagnosticAnalyzer::add_navigation_message(const NavigationMessage& nav) {
    summary_.nav_messages_total++;
    summary_.nav_by_constellation[nav.constellation]++;
}

void DiagnosticAnalyzer::add_pvt_solution(const ReceiverPVT& pvt) {
    summary_.pvt_solutions++;
}

const DataSummary& DiagnosticAnalyzer::get_summary() const {
    return summary_;
}

void DiagnosticAnalyzer::reset() {
    summary_ = DataSummary{};
    first_epoch_ = true;
}

void DiagnosticAnalyzer::update_time_span(int week, double tow) {
    summary_.end_gps_week = week;
    summary_.end_tow_s = tow;
    
    // Calculate duration
    double start_time = summary_.start_gps_week * 604800.0 + summary_.start_tow_s;
    double end_time = summary_.end_gps_week * 604800.0 + summary_.end_tow_s;
    summary_.duration_s = end_time - start_time;
}

void DiagnosticAnalyzer::print_summary() const {
    std::cout << "=== Data Summary ===\n";
    
    // Time span
    std::cout << "Time span:\n";
    std::cout << "  Start: GPS Week " << summary_.start_gps_week
              << ", TOW " << std::fixed << std::setprecision(3) << summary_.start_tow_s << " s\n";
    std::cout << "  End:   GPS Week " << summary_.end_gps_week
              << ", TOW " << std::fixed << std::setprecision(3) << summary_.end_tow_s << " s\n";
    std::cout << "  Duration: " << std::fixed << std::setprecision(3) << summary_.duration_s << " s\n";
    std::cout << "\n";
    
    // Epoch counts
    std::cout << "Epochs:\n";
    std::cout << "  Total epochs: " << summary_.total_epochs << "\n";
    if (summary_.total_epochs > 0) {
        double rate = summary_.duration_s / summary_.total_epochs;
        std::cout << "  Average rate: " << std::fixed << std::setprecision(2) << (1.0 / rate) << " Hz\n";
    }
    std::cout << "\n";
    
    // Observation counts
    std::cout << "Observations:\n";
    std::cout << "  Total measurements: " << summary_.total_observations << "\n";
    std::cout << "  Valid pseudorange:  " << summary_.valid_pseudorange << "\n";
    std::cout << "  Valid carrier phase: " << summary_.valid_carrier_phase << "\n";
    std::cout << "  Valid Doppler:      " << summary_.valid_doppler << "\n";
    std::cout << "\n";
    
    // By constellation
    if (!summary_.obs_by_constellation.empty()) {
        std::cout << "Observations by constellation:\n";
        for (const auto& [constellation, count] : summary_.obs_by_constellation) {
            std::cout << "  " << std::setw(10) << std::left << constellation_to_string(constellation)
                      << ": " << count << " obs";
            
            auto it = summary_.satellites_seen.find(constellation);
            if (it != summary_.satellites_seen.end()) {
                std::cout << ", " << it->second << " satellites";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
    
    // Signals
    if (!summary_.signals.empty()) {
        std::cout << "Signals observed:\n";
        for (const auto& sig : summary_.signals) {
            std::cout << "  " << std::setw(10) << std::left << constellation_to_string(sig.constellation)
                      << " SigID " << std::setw(2) << static_cast<int>(sig.signal_id)
                      << " (" << std::setw(15) << std::left << sig.signal_name << ")"
                      << ": " << sig.count << " obs\n";
        }
        std::cout << "\n";
    }
    
    // Navigation messages
    if (summary_.nav_messages_total > 0) {
        std::cout << "Navigation messages:\n";
        std::cout << "  Total: " << summary_.nav_messages_total << "\n";
        for (const auto& [constellation, count] : summary_.nav_by_constellation) {
            std::cout << "  " << std::setw(10) << std::left << constellation_to_string(constellation)
                      << ": " << count << "\n";
        }
        std::cout << "\n";
    }
    
    // PVT solutions
    if (summary_.pvt_solutions > 0) {
        std::cout << "PVT solutions: " << summary_.pvt_solutions << "\n\n";
    }
}

bool CsvExporter::export_observations(
    const std::string& filename,
    const std::vector<ObservationEpoch>& epochs)
{
    std::ofstream file(filename);
    if (!file) {
        return false;
    }
    
    // Write header
    file << "GPS_Week,TOW_s,Constellation,SatID,SignalID,FreqID,"
         << "Pseudorange_m,PR_Valid,Carrier_Phase_cy,CP_Valid,Doppler_Hz,DO_Valid,"
         << "CN0_dBHz,LockTime_raw,PR_StdDev,CP_StdDev,DO_StdDev,"
         << "HalfCycleValid,HalfCycleSubtracted\n";
    
    // Write observations
    for (const auto& epoch : epochs) {
        for (const auto& obs : epoch.observations) {
            file << epoch.gps_week << ","
                 << std::fixed << std::setprecision(6) << epoch.time_of_week_s << ","
                 << constellation_to_string(obs.constellation) << ","
                 << static_cast<int>(obs.satellite_id) << ","
                 << static_cast<int>(obs.signal_id) << ","
                 << static_cast<int>(obs.frequency_id) << ","
                 << std::fixed << std::setprecision(3) << obs.pseudorange_m << ","
                 << (obs.pseudorange_valid ? 1 : 0) << ","
                 << std::fixed << std::setprecision(3) << obs.carrier_phase_cycles << ","
                 << (obs.carrier_phase_valid ? 1 : 0) << ","
                 << std::fixed << std::setprecision(3) << obs.doppler_hz << ","
                 << (obs.doppler_valid ? 1 : 0) << ","
                 << std::fixed << std::setprecision(1) << obs.cn0_db_hz << ","
                 << obs.lock_time_raw << ","
                 << static_cast<int>(obs.pr_stdev) << ","
                 << static_cast<int>(obs.cp_stdev) << ","
                 << static_cast<int>(obs.do_stdev) << ","
                 << (obs.half_cycle_valid ? 1 : 0) << ","
                 << (obs.half_cycle_subtracted ? 1 : 0) << "\n";
        }
    }
    
    return true;
}

bool CsvExporter::export_navigation_messages(
    const std::string& filename,
    const std::vector<NavigationMessage>& messages)
{
    std::ofstream file(filename);
    if (!file) {
        return false;
    }
    
    // Write header
    file << "Constellation,SatID,SignalID,FreqID,NumWords,Words\n";
    
    // Write navigation messages
    for (const auto& nav : messages) {
        file << constellation_to_string(nav.constellation) << ","
             << static_cast<int>(nav.satellite_id) << ","
             << static_cast<int>(nav.signal_id) << ","
             << static_cast<int>(nav.frequency_id) << ","
             << static_cast<int>(nav.num_words) << ",\"";
        
        // Write words as hex string
        for (size_t i = 0; i < nav.words.size(); ++i) {
            if (i > 0) file << " ";
            file << std::hex << std::setw(8) << std::setfill('0') << nav.words[i];
        }
        file << std::dec << std::setfill(' ') << "\"\n";
    }
    
    return true;
}

bool CsvExporter::export_pvt_solutions(
    const std::string& filename,
    const std::vector<ReceiverPVT>& solutions)
{
    std::ofstream file(filename);
    if (!file) {
        return false;
    }
    
    // Write header
    file << "Year,Month,Day,Hour,Minute,Second,Nanosecond,TimeValid,"
         << "Longitude_deg,Latitude_deg,Height_m,HeightMSL_m,"
         << "HorizontalAcc_m,VerticalAcc_m,"
         << "VelNorth_m_s,VelEast_m_s,VelDown_m_s,"
         << "GroundSpeed_m_s,Heading_deg,SpeedAcc_m_s,HeadingAcc_deg,"
         << "FixType,NumSV,PDOP,GnssFixOK,DiffSoln,CarrSoln\n";
    
    // Write PVT solutions
    for (const auto& pvt : solutions) {
        file << pvt.year << ","
             << static_cast<int>(pvt.month) << ","
             << static_cast<int>(pvt.day) << ","
             << static_cast<int>(pvt.hour) << ","
             << static_cast<int>(pvt.minute) << ","
             << static_cast<int>(pvt.second) << ","
             << pvt.nanosecond << ","
             << (pvt.time_valid ? 1 : 0) << ","
             << std::fixed << std::setprecision(9) << pvt.longitude_deg << ","
             << std::fixed << std::setprecision(9) << pvt.latitude_deg << ","
             << std::fixed << std::setprecision(3) << pvt.height_m << ","
             << std::fixed << std::setprecision(3) << pvt.height_msl_m << ","
             << std::fixed << std::setprecision(3) << pvt.horizontal_accuracy_m << ","
             << std::fixed << std::setprecision(3) << pvt.vertical_accuracy_m << ","
             << std::fixed << std::setprecision(3) << pvt.vel_north_m_s << ","
             << std::fixed << std::setprecision(3) << pvt.vel_east_m_s << ","
             << std::fixed << std::setprecision(3) << pvt.vel_down_m_s << ","
             << std::fixed << std::setprecision(3) << pvt.ground_speed_m_s << ","
             << std::fixed << std::setprecision(5) << pvt.heading_deg << ","
             << std::fixed << std::setprecision(3) << pvt.speed_accuracy_m_s << ","
             << std::fixed << std::setprecision(5) << pvt.heading_accuracy_deg << ","
             << static_cast<int>(pvt.fix_type) << ","
             << static_cast<int>(pvt.num_satellites) << ","
             << std::fixed << std::setprecision(2) << (pvt.pdop * 0.01) << ","
             << (pvt.gnss_fix_ok ? 1 : 0) << ","
             << (pvt.diff_soln ? 1 : 0) << ","
             << static_cast<int>(pvt.carr_soln) << "\n";
    }
    
    return true;
}

} // namespace gnss
