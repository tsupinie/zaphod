
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

std::chrono::duration<long long> zaphod::duration_units(const g2int val) {
    switch(val) {
        case 0: return 1min;
        case 1: return 1h;
        case 2: return 24h;
        case 13: return 1s;
        case 255: return -1s;
        default: throw std::runtime_error("Unhandled time units");
    }
}

std::chrono::duration<long long> zaphod::duration_from_buffer(const g2int* buf) {
    int fcst_time_raw = buf[1];
    std::chrono::duration<long long> dur = duration_units(buf[0]);

    return dur < 0s ? dur : dur * fcst_time_raw;
}

std::string zaphod::time_point_to_string(const std::chrono::system_clock::time_point& tp, const std::string& format) {
    char time_str[100] = {};

    std::time_t tp_c = std::chrono::system_clock::to_time_t(tp);
    std::strftime(time_str, sizeof(time_str), format.c_str(), std::localtime(&tp_c));

    return time_str;
}