
#include <iostream>
#include <vector>

#include <zaphod.h>

#include <proj/crs.hpp>
#include <proj/util.hpp>

using namespace zaphod;

template <typename T>
std::ostream& operator<<(std::ostream& stream, std::vector<T> vec) {
    if (vec.size() > 10) {
        for (size_t i = 0; i < 5; i++) {
            stream << vec[i] << ", ";
        }

        stream << "..., ";

        for (size_t i = vec.size() - 5; i < vec.size(); i++) {
            stream << vec[i];

            if (i < vec.size() - 1)
                stream << ", ";
        }

        stream << std::endl;
    }
    else {
        for (size_t i = 0; i < vec.size(); i++) {
            stream << vec[i];

            if (i < vec.size() - 1)
                stream << ", ";
        }

        stream << std::endl;
    }

    return stream;
}

int main() {
    Grib2EarthShapeDescriptor earth_shape(6371229., 6371229., true);
    Grib2SpatialGridDescriptor grid = {1799, 1099};
    Grib2ScanFlagsDescriptor scan_flags = {false, true, true, false};
    Grib2LambertProjectionDescriptor lcc_projection = {38.5, -97.5, 38.5, 38.5, 21.138123, -122.719528, 3000., 3000.};

    Grib2GridDefLambert lcc({earth_shape, grid, lcc_projection, scan_flags});

    PJ_CONTEXT *ctx = proj_context_create();
    auto trans = lcc.get_fwd_transform(ctx);

    float x, y;
    std::tie(x, y) = transform_point(-97.44, 35.18, trans);

    std::cout << x << " " << y << std::endl;

    proj_context_destroy(ctx);

    std::vector<float> xs = lcc.get_xs();
    std::vector<float> ys = lcc.get_ys();

    std::cout << xs;
    std::cout << ys;

    std::vector<float> lats, lons;
    std::tie(lons, lats) = lcc.get_lonlats();

    std::cout << lons;
    std::cout << lats;

    grid = {121, 81};
    Grib2LatLonProjectionDescriptor latlon_projection = {20., -125., 60., -65., 0.5, 0.5};
    Grib2GridDefLatLon latlon({earth_shape, grid, latlon_projection, scan_flags});
    xs = latlon.get_xs();
    ys = latlon.get_ys();

    std::cout << xs;
    std::cout << ys;

    return 0;
}