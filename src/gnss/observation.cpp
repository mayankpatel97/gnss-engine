#include "gnss/observation.hpp"
#include <algorithm>

namespace gnss {

const char* constellation_to_string(GnssConstellation constellation) {
    switch (constellation) {
        case GnssConstellation::GPS: return "GPS";
        case GnssConstellation::SBAS: return "SBAS";
        case GnssConstellation::Galileo: return "Galileo";
        case GnssConstellation::BeiDou: return "BeiDou";
        case GnssConstellation::IMES: return "IMES";
        case GnssConstellation::QZSS: return "QZSS";
        case GnssConstellation::GLONASS: return "GLONASS";
        default: return "Unknown";
    }
}

GnssObservation::GnssObservation()
    : constellation(GnssConstellation::Unknown)
    , satellite_id(0)
    , signal_id(0)
    , frequency_id(0)
    , pseudorange_m(0.0)
    , carrier_phase_cycles(0.0)
    , doppler_hz(0.0)
    , cn0_db_hz(0.0)
    , lock_time_raw(0)
    , pr_stdev(255)
    , cp_stdev(255)
    , do_stdev(255)
    , pseudorange_valid(false)
    , carrier_phase_valid(false)
    , doppler_valid(false)
    , half_cycle_valid(false)
    , half_cycle_subtracted(false)
    , carrier_phase_slip(false)
{
}

ObservationEpoch::ObservationEpoch()
    : gps_week(0)
    , time_of_week_s(0.0)
    , leap_seconds(0)
    , leap_seconds_valid(false)
{
}

size_t ObservationEpoch::count_by_constellation(GnssConstellation constellation) const {
    return std::count_if(observations.begin(), observations.end(),
        [constellation](const GnssObservation& obs) {
            return obs.constellation == constellation;
        });
}

NavigationMessage::NavigationMessage()
    : constellation(GnssConstellation::Unknown)
    , satellite_id(0)
    , signal_id(0)
    , frequency_id(0)
    , num_words(0)
    , version(0)
    , reserved1(0)
{
}

ReceiverPVT::ReceiverPVT()
    : year(0)
    , month(0)
    , day(0)
    , hour(0)
    , minute(0)
    , second(0)
    , nanosecond(0)
    , time_valid(false)
    , longitude_deg(0.0)
    , latitude_deg(0.0)
    , height_m(0.0)
    , height_msl_m(0.0)
    , horizontal_accuracy_m(0.0)
    , vertical_accuracy_m(0.0)
    , vel_north_m_s(0.0)
    , vel_east_m_s(0.0)
    , vel_down_m_s(0.0)
    , ground_speed_m_s(0.0)
    , heading_deg(0.0)
    , speed_accuracy_m_s(0.0)
    , heading_accuracy_deg(0.0)
    , fix_type(0)
    , num_satellites(0)
    , pdop(9999)
    , gnss_fix_ok(false)
    , diff_soln(false)
    , carr_soln(0)
{
}

} // namespace gnss
