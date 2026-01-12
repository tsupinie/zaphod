
#include <iostream>
#include <vector>

#include <grib2_grid_templates.cpp>

#include <proj/crs.hpp>
#include <proj/util.hpp>

template <typename T>
void print_ary_sample(std::vector<T> ary) {
    for (size_t i = 0; i < 5; i++) {
        std::cout << ary[i] << ", ";
    }

    std::cout << "..." << std::endl;
}

int main() {
    Grib2GridDefEarthShape earth_shape(6371229., 6371229., true);
    Grib2GridDefSpatialGrid grid = {1799, 1099};
    Grib2GridDefScanFlags scan_flags = {false, true, true, false};

    Grib2GridDefLambert lcc(earth_shape, grid, scan_flags, 38.5, -97.5, 38.5, 38.5, 21.138123, -122.719528, 3000., 3000.);

    auto crs = lcc.get_crs();
    auto crs_base = crs->baseCRS();

    PJ_CONTEXT *ctx = proj_context_create();

    float x, y;
    std::tie(x, y) = transform_point(-97.44, 35.18, crs_base, crs, ctx);

    std::cout << x << " " << y << std::endl;

    proj_context_destroy(ctx);

    std::vector<float> xs = lcc.get_xs();
    std::vector<float> ys = lcc.get_ys();

    print_ary_sample(xs);
    print_ary_sample(ys);

    Grib2GridDefLatLon latlon(earth_shape, grid, scan_flags, 20., -125., 60., -65., 0.5, 0.5);
    xs = latlon.get_xs();
    ys = latlon.get_ys();

    print_ary_sample(xs);
    print_ary_sample(ys);

    return 0;
}