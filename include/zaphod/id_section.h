#ifndef __ZAPHOD_ID_SECTION__
#define __ZAPHOD_ID_SECTION__

extern "C" {
    #include "grib2.h"
}

#include <iostream>
#include <chrono>

namespace zaphod {

struct Grib2IdSection {
    g2int center_id;
    g2int subcenter_id;
    g2int master_table_version;
    g2int local_table_version;
    g2int ref_time_significance;
    std::chrono::system_clock::time_point ref_time;
    g2int production_status;
    g2int type_of_data;

    static Grib2IdSection from_buffer(g2int* buf);
    std::chrono::system_clock::time_point init_datetime(std::chrono::system_clock::duration fcst_time) const;
};

std::ostream& operator<<(std::ostream& stream, const Grib2IdSection& idsect);

}

#endif