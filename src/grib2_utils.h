#ifndef __ZAPHOD_UTILS__
#define __ZAPHOD_UTILS__

extern "C" {
    #include <grib2.h>
}

#include <chrono>

namespace zaphod {

float value_from_buffer(const g2int* buf);
std::chrono::system_clock::time_point time_point_from_buffer(const g2int* buf);
std::chrono::duration<unsigned int> duration_from_buffer(const g2int* buf);

}

#endif