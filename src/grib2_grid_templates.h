#ifndef __SPCPOST_GRIB2_GRID_TEMPLATES__
#define __SPCPOST_GRIB2_GRID_TEMPLATES__

extern "C" {
    #include <grib2.h>
}

#include <map>
#include <type_traits>
#include <any>
#include <tuple>
#include <memory>
#include <vector>

struct Grib2GridDef {
    virtual std::string get_proj_type() = 0;
    virtual std::map<std::string, float> get_proj_parameters() = 0;
    // TAS: maybe later. Might be too complicated to implement in C++ in a limited time
    // virtual std::tuple<arr_out_2d_t, arr_out_2d_t> get_latlons() = 0;
    virtual std::vector<float> get_xs() = 0;
    virtual std::vector<float> get_ys() = 0;

    static std::shared_ptr<Grib2GridDef> select_grid_def_template(g2int template_num, g2int* template_buf);
};

struct Grib2GridDefEarthShape {
    float earth_semimajor;
    float earth_semiminor;
    bool earth_is_sphere;

    static Grib2GridDefEarthShape from_buffer(g2int* buf);
};

template<class, class=void>
struct has_earth_shape : std::false_type {};

template<class T>
struct has_earth_shape<T,
    std::void_t<
        decltype(std::is_default_constructible_v<T>),
        decltype(T{}.earth_shape)
    >
> : std::true_type {};

template <typename T>
inline constexpr bool has_earth_shape_v = has_earth_shape<T>::value;

struct Grib2GridDefSpatialGrid {
    unsigned int ni;
    unsigned int nj;

    static Grib2GridDefSpatialGrid from_buffer(g2int* buf);
};

struct Grib2GridDefScanFlags {
    bool do_adjacent_rows_alternate;
    bool first_row_is_south;
    bool first_col_is_west;
    bool ary_is_column_major;

    static Grib2GridDefScanFlags from_buffer(g2int* buf);
};

struct Grib2GridDefLatLon : public Grib2GridDef {
    Grib2GridDefEarthShape earth_shape;
    Grib2GridDefSpatialGrid grid;
    Grib2GridDefScanFlags scan_flags;

    float latitude_first;
    float longitude_first;
    float latitude_last;
    float longitude_last;
    float di;
    float dj;

    static Grib2GridDefLatLon from_buffer(g2int* buf);

    std::map<std::string, float> get_proj_parameters();
    std::string get_proj_type() { return "latlon"; };
    std::vector<float> get_xs();
    std::vector<float> get_ys();
};

struct Grib2GridDefLambert : public Grib2GridDef {
    Grib2GridDefEarthShape earth_shape;
    Grib2GridDefSpatialGrid grid;
    Grib2GridDefScanFlags scan_flags;

    float center_latitude;
    float standard_longitude;
    float standard_latitude_1;
    float standard_latitude_2;
    float latitude_first;
    float longitude_first;
    float di;
    float dj;

    static Grib2GridDefLambert from_buffer(g2int* buf);

    std::map<std::string, float> get_proj_parameters();
    std::string get_proj_type() { return "lcc"; };
    std::vector<float> get_xs();
    std::vector<float> get_ys();
};

#endif