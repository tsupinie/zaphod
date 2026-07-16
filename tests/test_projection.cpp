
#include <iostream>
#include <vector>

#include <zaphod.h>

#include <proj/crs.hpp>
#include <proj/util.hpp>

#include "utils.h"

using namespace zaphod;

int main() {
    Grib2EarthShapeDescriptor earth_shape(6371229., 6371229., true);
    Grib2SpatialGridDescriptor grid = {1799, 1099};
    Grib2ScanFlagsDescriptor scan_flags = {false, true, true, false};
    Grib2LambertProjectionDescriptor lcc_projection = {21.138123, -122.719528, 38.5, -97.5, 38.5, 38.5, 3000., 3000.};

    Grib2GridDefLambert lcc({earth_shape, grid, lcc_projection, scan_flags});

    PJ_CONTEXT *ctx = proj_context_create();
    auto trans = lcc.get_fwd_transform(ctx);

    float x, y;
    std::tie(x, y) = transform_point(-97.44, 35.18, trans);

    std::cout << x << " " << y << std::endl;

    proj_context_destroy(ctx);

    std::vector<float> xs = lcc.get_xs();
    std::vector<float> ys = lcc.get_ys();

    std::cout << xs << std::endl;
    std::cout << ys << std::endl;

    std::vector<float> lats, lons;
    std::tie(lons, lats) = lcc.get_lonlats();

    std::cout << lons << std::endl;
    std::cout << lats << std::endl;

    grid = {121, 81};
    Grib2LatLonProjectionDescriptor latlon_projection = {20., -125., 60., -65., 0.5, 0.5};
    Grib2GridDefLatLon latlon({earth_shape, grid, latlon_projection, scan_flags});
    xs = latlon.get_xs();
    ys = latlon.get_ys();

    std::cout << xs << std::endl;
    std::cout << ys << std::endl;

    return 0;
}