
#include <cmath>
#include <iostream>

#include "grib2_grid_templates.h"


std::shared_ptr<Grib2GridDef> Grib2GridDef::select_grid_def_template(g2int template_num, g2int* template_buf) {
    switch (template_num) {
        case 0:
            return std::make_shared<Grib2GridDefLatLon>(Grib2GridDefLatLon::from_buffer(template_buf));
        case 30:
            return std::make_shared<Grib2GridDefLambert>(Grib2GridDefLambert::from_buffer(template_buf));
        default:
            throw "Unknown grid template number";
    }
}


Grib2GridDefEarthShape Grib2GridDefEarthShape::from_buffer(g2int* buf) {
    unsigned char earth_shape = buf[0];
    float earth_radius_src = pow(10, -buf[1]) * buf[2];
    float earth_semimajor_src = pow(10, -buf[3]) * buf[4];
    float earth_semiminor_src = pow(10, -buf[5]) * buf[6];

    Grib2GridDefEarthShape shp;

    switch (earth_shape) {
        case 0:
            shp.earth_semimajor = shp.earth_semiminor = 6367470.f;
            shp.earth_is_sphere = true;
            break;
        case 1:
            shp.earth_semimajor = shp.earth_semiminor = earth_radius_src;
            shp.earth_is_sphere = true;
            break;
        case 2:
            shp.earth_semimajor = 6378160.0;
            shp.earth_semiminor = 6356775.0;
            shp.earth_is_sphere = false;
            break;
        case 3:
            shp.earth_semimajor = earth_semimajor_src * 1000;
            shp.earth_semiminor = earth_semiminor_src * 1000;
            shp.earth_is_sphere = false;
            break;
        case 4:
        case 5:
            shp.earth_semimajor = 6378137.0;
            shp.earth_semiminor = 6356752.314;
            shp.earth_is_sphere = false;
            break;
        case 6:
            shp.earth_semimajor = shp.earth_semiminor = 6371229.0;
            shp.earth_is_sphere = true;
            break;
        case 7:
            shp.earth_semimajor = earth_semimajor_src;
            shp.earth_semiminor = earth_semiminor_src;
            shp.earth_is_sphere = false;
            break;
        default:
            throw "Unknown earth shape";
    }

    return shp;
}

Grib2GridDefSpatialGrid Grib2GridDefSpatialGrid::from_buffer(g2int* buf) {
    Grib2GridDefSpatialGrid grd;
    grd.ni = buf[0];
    grd.nj = buf[1];
    return grd;
}

Grib2GridDefScanFlags Grib2GridDefScanFlags::from_buffer(g2int* buf) {
    Grib2GridDefScanFlags flags;
    flags.do_adjacent_rows_alternate = buf[0] & (1 << 4);
    flags.first_row_is_south = buf[0] & (1 << 6);
    flags.first_col_is_west = !(buf[0] & (1 << 7));
    flags.ary_is_column_major = buf[0] & (1 << 5);

    return flags;
}

Grib2GridDefLatLon Grib2GridDefLatLon::from_buffer(g2int* buf) {
    Grib2GridDefLatLon def;
    def.earth_shape = Grib2GridDefEarthShape::from_buffer(buf);
    def.grid = Grib2GridDefSpatialGrid::from_buffer(buf + 7);
    def.scan_flags = Grib2GridDefScanFlags::from_buffer(buf + 18);

    // Should take into account the basic angle division from the file at some point. For now, just assume 1e-6.
    const float angle_unit = 1e-6;

    def.latitude_first = buf[11] * angle_unit;
    def.longitude_first = buf[12] * angle_unit;
    def.latitude_last = buf[14] * angle_unit;
    def.longitude_last = buf[15] * angle_unit;
    def.di = buf[16] * angle_unit;
    def.dj = buf[17] * angle_unit;

    return def;
}

std::map<std::string, float> Grib2GridDefLatLon::get_proj_parameters() {
    std::map<std::string, float> params;
    params["a"] = this->earth_shape.earth_semimajor;
    params["b"] = this->earth_shape.earth_semiminor;
    return params;
}

std::vector<float> Grib2GridDefLatLon::get_xs() {
    std::vector<float> xs(this->grid.ni);
    const float inc_sign = this->scan_flags.first_col_is_west ? 1 : -1;

    for (size_t i = 0; i < this->grid.ni; i++) {
        xs[i] = this->longitude_first + inc_sign * i * this->di;
    }

    return xs;
}

std::vector<float> Grib2GridDefLatLon::get_ys() {
    std::vector<float> ys(this->grid.nj);
    const float inc_sign = this->scan_flags.first_row_is_south ? 1 : -1;

    for (size_t j = 0; j < this->grid.nj; j++) {
        ys[j] = this->latitude_first + inc_sign * j * this->dj;
    }

    return ys;
}


Grib2GridDefLambert Grib2GridDefLambert::from_buffer(g2int* buf) {
    Grib2GridDefLambert def;
    def.earth_shape = Grib2GridDefEarthShape::from_buffer(buf);
    def.grid = Grib2GridDefSpatialGrid::from_buffer(buf + 7);
    def.scan_flags = Grib2GridDefScanFlags::from_buffer(buf + 17);

    const float angle_unit = 1e-6;
    const float spacing_unit = 1e-3;

    def.latitude_first = buf[9] * angle_unit;
    def.longitude_first = buf[10] * angle_unit;
    def.center_latitude = buf[12] * angle_unit;
    def.standard_longitude = buf[13] * angle_unit;
    def.standard_latitude_1 = buf[18] * angle_unit;
    def.standard_latitude_2 = buf[19] * angle_unit;
    def.di = buf[14] * spacing_unit;
    def.dj = buf[15] * spacing_unit;

    return def;
}

std::map<std::string, float> Grib2GridDefLambert::get_proj_parameters() {
    std::map<std::string, float> params;
    params["lon_0"] = this->standard_longitude;
    params["lat_0"] = this->center_latitude;
    params["lat_1"] = this->standard_latitude_1;
    params["lat_2"] = this->standard_latitude_2;
    params["lat_first"] = this->latitude_first;
    params["lon_first"] = this->longitude_first;
    params["a"] = this->earth_shape.earth_semimajor;
    params["b"] = this->earth_shape.earth_semiminor;
    return params;
}

std::vector<float> Grib2GridDefLambert::get_xs() {
    // XXX: This assumes 0, 0 is at the center of the grid, which is not the case in general. In general, need to use
    //  latitude_first and longitude_first to find the corner, but this requires proj, which I don't want to deal with
    //  right now.
    std::vector<float> xs(this->grid.ni);
    const float inc_sign = this->scan_flags.first_col_is_west ? 1 : -1;

    for (size_t i = 0; i < this->grid.ni; i++) {
        xs[i] = inc_sign * (i - (this->grid.ni - 1) / 2.f) * this->di;
    }

    return xs;
}

std::vector<float> Grib2GridDefLambert::get_ys() {
    // XXX: This assumes 0, 0 is at the center of the grid, which is not the case in general. In general, need to use
    //  latitude_first and longitude_first to find the corner, but this requires proj, which I don't want to deal with
    //  right now.
    std::vector<float> ys(this->grid.nj);
    const float inc_sign = this->scan_flags.first_row_is_south ? 1 : -1;

    for (size_t j = 0; j < this->grid.nj; j++) {
        ys[j] = inc_sign * (j - (this->grid.nj - 1) / 2.f) * this->dj;
    }

    return ys;
}