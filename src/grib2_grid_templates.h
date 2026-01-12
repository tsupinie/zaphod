#ifndef __SPCPOST_GRIB2_GRID_TEMPLATES__
#define __SPCPOST_GRIB2_GRID_TEMPLATES__

extern "C" {
    #include <grib2.h>
}

#include <proj/util.hpp>
#include <proj/coordinatesystem.hpp>
#include <proj/datum.hpp>
#include <proj/crs.hpp>

#include <map>
#include <type_traits>
#include <any>
#include <tuple>
#include <memory>
#include <vector>

struct Grib2GridDef {
    virtual std::string get_proj_type() = 0;
    virtual std::map<std::string, float> get_proj_parameters() = 0;

    
    virtual std::tuple<std::vector<float>, std::vector<float>> get_latlons() = 0;
    virtual std::vector<float> get_xs() = 0;
    virtual std::vector<float> get_ys() = 0;

    static std::shared_ptr<Grib2GridDef> select_grid_def_template(g2int template_num, g2int* template_buf);

    NS_PROJ::operation::CoordinateTransformerNNPtr get_fwd_transform(PJ_CONTEXT* ctx);
    NS_PROJ::operation::CoordinateTransformerNNPtr get_inv_transform(PJ_CONTEXT* ctx);

    protected:
    virtual NS_PROJ::crs::ProjectedCRSNNPtr get_crs() = 0;
};

struct Grib2GridDefEarthShape {
    Grib2GridDefEarthShape() = delete;
    Grib2GridDefEarthShape(float semimajor, float semiminor, bool is_sphere) : 
        earth_semimajor(semimajor), earth_semiminor(semiminor), earth_is_sphere(is_sphere) {}

    float earth_semimajor;
    float earth_semiminor;
    bool earth_is_sphere;

    static Grib2GridDefEarthShape from_buffer(g2int* buf);
    NS_PROJ::datum::EllipsoidNNPtr get_proj_ellipsoid();
};

template<class, class=void>
struct has_earth_shape : std::false_type {};

template<class T>
struct has_earth_shape<T,
    std::void_t<
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
    Grib2GridDefLatLon() = delete;

    Grib2GridDefLatLon(
        const Grib2GridDefEarthShape& earth_shape, const Grib2GridDefSpatialGrid& grid, const Grib2GridDefScanFlags& scan_flags,
        const float latitude_first, const float longitude_first,
        const float latitude_last, const float longitude_last,
        const float di, const float dj) : 
            earth_shape(earth_shape), grid(grid), scan_flags(scan_flags), 
            latitude_first(latitude_first), longitude_first(longitude_first),
            latitude_last(latitude_last), longitude_last(longitude_last),
            di(di), dj(dj) {}

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
    std::tuple<std::vector<float>, std::vector<float>> get_latlons();
    std::vector<float> get_xs();
    std::vector<float> get_ys();

    protected:
    NS_PROJ::crs::ProjectedCRSNNPtr get_crs();
};

struct Grib2GridDefLambert : public Grib2GridDef {
    Grib2GridDefLambert() = delete;

    Grib2GridDefLambert(
    const Grib2GridDefEarthShape& earth_shape, const Grib2GridDefSpatialGrid& grid, const Grib2GridDefScanFlags& scan_flags,
    const float center_latitude, const float standard_longitude,
    const float standard_latitude_1, const float standard_latitude_2,
    const float latitude_first, const float longitude_first,
    const float di, const float dj) : 
        earth_shape(earth_shape), grid(grid), scan_flags(scan_flags),
        center_latitude(center_latitude), standard_longitude(standard_longitude),
        standard_latitude_1(standard_latitude_1), standard_latitude_2(standard_latitude_2),
        latitude_first(latitude_first), longitude_first(longitude_first),
        di(di), dj(dj) {}

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
    std::tuple<std::vector<float>, std::vector<float>> get_latlons();
    std::vector<float> get_xs();
    std::vector<float> get_ys();

    protected:
    NS_PROJ::crs::ProjectedCRSNNPtr get_crs();
};

std::tuple<float, float> transform_point(float x_in, float y_in, const NS_PROJ::operation::CoordinateTransformerNNPtr& trans);
NS_PROJ::operation::CoordinateTransformerNNPtr make_transformer(NS_PROJ::crs::CRSNNPtr crs_from, NS_PROJ::crs::CRSNNPtr crs_to, PJ_CONTEXT* ctx);

#endif