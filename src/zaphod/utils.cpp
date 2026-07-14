
#include <zaphod/utils.h>

using namespace std::chrono_literals;

float zaphod::value_from_buffer(const g2int* buf) {
    return pow(10, -buf[0]) * buf[1];
}

std::chrono::system_clock::time_point zaphod::time_point_from_buffer(const g2int* buf) {
    std::tm t{};

    t.tm_year = buf[0] - 1900;
    t.tm_mon = buf[1] - 1;
    t.tm_mday = buf[2];
    t.tm_hour = buf[3];
    t.tm_min = buf[4];
    t.tm_sec = buf[5];
    t.tm_isdst = -1; // Well, that's a few hours I'll never get back

    return std::chrono::system_clock::from_time_t(std::mktime(&t));
}

std::chrono::duration<unsigned int> zaphod::duration_from_buffer(const g2int* buf) {
    unsigned int units = buf[0];
    int fcst_time_raw = buf[1];
    
    std::chrono::duration<unsigned int> dur;

    switch(units) {
        case 0: dur = fcst_time_raw * 1min; break;
        case 1: dur = fcst_time_raw * 1h; break;
        case 2: dur = fcst_time_raw * 24h; break;
        case 13: dur = fcst_time_raw * 1s; break;
        default: throw std::runtime_error("Unhandled time units");
    }

    return dur;
}