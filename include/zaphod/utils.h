#ifndef __ZAPHOD_UTILS__
#define __ZAPHOD_UTILS__

extern "C" {
    #include <grib2.h>
}

#include <chrono>
#include <cmath>
#include <stdexcept>

namespace zaphod {

float value_from_buffer(const g2int* buf);
std::chrono::system_clock::time_point time_point_from_buffer(const g2int* buf);
std::chrono::duration<unsigned int> duration_from_buffer(const g2int* buf);

class ParameterNotInMessage : public std::runtime_error {
    public:
    ParameterNotInMessage(std::string what) : std::runtime_error(what) {}
};

}

#endif