
#include <iostream>

#include <grib2_grid_templates.cpp>

#include <proj/crs.hpp>
#include <proj/util.hpp>

int main() {
    Grib2GridDefEarthShape earth_shape(6371229., 6271229., true);
    Grib2GridDefSpatialGrid grid = {1799, 1099};
    Grib2GridDefScanFlags scan_flags = {false, false, false, false};

    Grib2GridDefLambert lcc(earth_shape, grid, scan_flags, 38.5, -97.5, 38.5, 38.5, 21.138123, -122.719528, 3000., 3000.);

    auto crs = lcc.get_crs();
    auto crs_base = crs->baseCRS();

    PJ_CONTEXT *ctx = proj_context_create();

    float x, y;
    std::tie(x, y) = transform_point(-97.44, 35.18, crs_base, crs, ctx);

    std::cout << x << " " << y << std::endl;

    proj_context_destroy(ctx);

    return 0;
}