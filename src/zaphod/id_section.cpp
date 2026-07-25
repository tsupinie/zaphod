
#include <iostream>

#include <zaphod/id_section.h>
#include <zaphod/utils.h>

using namespace zaphod;

Grib2IdSection Grib2IdSection::from_buffer(g2int* buf) {
    return {
        buf[0],
        buf[1],
        buf[2],
        buf[3],
        buf[4],
        time_point_from_buffer(buf + 5),
        buf[11],
        buf[12]
    };
}

std::chrono::system_clock::time_point Grib2IdSection::init_datetime(std::chrono::system_clock::duration fcst_time) const {
    if (this->ref_time_significance == 0 || this->ref_time_significance == 1) {
        return this->ref_time;
    }

    return this->ref_time - fcst_time;
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2IdSection& idsect) {
    std::string time_str = time_point_to_string(idsect.ref_time, "%Y-%m-%dT%H:%M:%S");

    stream << "{" << idsect.center_id << ", " << idsect.subcenter_id << ", " << idsect.master_table_version << ", " << idsect.local_table_version
           << ", " << idsect.ref_time_significance << ", " << time_str << ", " << idsect.production_status << ", " << idsect.type_of_data << "}";

    return stream;
}