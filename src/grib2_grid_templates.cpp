
#include <cmath>
#include <iostream>
#include <chrono>

#include "grib2_grid_templates.h"

using namespace zaphod;


NS_PROJ::crs::CRSNNPtr make_proj_crs(
    const NS_PROJ::datum::EllipsoidNNPtr& ellipsoid, 
    const NS_PROJ::operation::ConversionNNPtr& conversion
) {
    NS_PROJ::util::PropertyMap props;
    props.set("name", "");

    auto earth_cs = NS_PROJ::cs::EllipsoidalCS::createLongitudeLatitude(NS_PROJ::common::UnitOfMeasure::DEGREE);

    auto prime_meridian = NS_PROJ::datum::PrimeMeridian::create(props, NS_PROJ::common::Angle(0));

    auto datum = NS_PROJ::datum::GeodeticReferenceFrame::create(
        props, ellipsoid, 
        NS_PROJ::util::optional(std::string()), 
        prime_meridian
    );

    auto proj_cs = NS_PROJ::cs::CartesianCS::createEastingNorthing(NS_PROJ::common::UnitOfMeasure::METRE);

    auto base_crs = NS_PROJ::crs::GeographicCRS::create(props, datum, earth_cs);
    return NS_PROJ::crs::ProjectedCRS::create(props, base_crs, conversion, proj_cs);
}

NS_PROJ::operation::CoordinateTransformerNNPtr zaphod::make_transformer(NS_PROJ::crs::CRSNNPtr crs_from, NS_PROJ::crs::CRSNNPtr crs_to, PJ_CONTEXT* ctx) {
    auto factory = NS_PROJ::operation::CoordinateOperationFactory::create();
    auto op = factory->createOperation(crs_from, crs_to);
    return op->coordinateTransformer(ctx);
}

std::tuple<float, float> zaphod::transform_point(float x_in, float y_in, const NS_PROJ::operation::CoordinateTransformerNNPtr& trans) {
    PJ_COORD coord = {{x_in, y_in, 0, HUGE_VAL}};
    PJ_COORD coord_trans = trans->transform(coord);

    return {coord_trans.xy.x, coord_trans.xy.y};
}


Grib2EarthShapeDescriptor Grib2EarthShapeDescriptor::from_buffer(const g2int* buf) {
    unsigned char earth_shape = buf[0];
    float earth_radius_src = pow(10, -buf[1]) * buf[2];
    float earth_semimajor_src = pow(10, -buf[3]) * buf[4];
    float earth_semiminor_src = pow(10, -buf[5]) * buf[6];

    float earth_semimajor, earth_semiminor;
    bool earth_is_sphere;

    switch (earth_shape) {
        case 0:
            earth_semimajor = earth_semiminor = 6367470.f;
            earth_is_sphere = true;
            break;
        case 1:
            earth_semimajor = earth_semiminor = earth_radius_src;
            earth_is_sphere = true;
            break;
        case 2:
            earth_semimajor = 6378160.0;
            earth_semiminor = 6356775.0;
            earth_is_sphere = false;
            break;
        case 3:
            earth_semimajor = earth_semimajor_src * 1000;
            earth_semiminor = earth_semiminor_src * 1000;
            earth_is_sphere = false;
            break;
        case 4:
        case 5:
            earth_semimajor = 6378137.0;
            earth_semiminor = 6356752.314;
            earth_is_sphere = false;
            break;
        case 6:
            earth_semimajor = earth_semiminor = 6371229.0;
            earth_is_sphere = true;
            break;
        case 7:
            earth_semimajor = earth_semimajor_src;
            earth_semiminor = earth_semiminor_src;
            earth_is_sphere = false;
            break;
        default:
            throw "Unknown earth shape";
    }

    return Grib2EarthShapeDescriptor(earth_semimajor, earth_semiminor, earth_is_sphere);
}

NS_PROJ::datum::EllipsoidNNPtr Grib2EarthShapeDescriptor::get_proj_ellipsoid() const {
    NS_PROJ::util::PropertyMap props;
    props.set("name", "");

    return NS_PROJ::datum::Ellipsoid::createTwoAxis(
        props,
        NS_PROJ::common::Length(this->earth_semimajor, NS_PROJ::common::UnitOfMeasure::METRE), 
        NS_PROJ::common::Length(this->earth_semiminor, NS_PROJ::common::UnitOfMeasure::METRE)
    );
}

Grib2SpatialGridDescriptor Grib2SpatialGridDescriptor::from_buffer(const g2int* buf) {
    Grib2SpatialGridDescriptor grd;
    grd.ni = buf[0];
    grd.nj = buf[1];
    return grd;
}

Grib2ScanFlagsDescriptor Grib2ScanFlagsDescriptor::from_buffer(const g2int* buf) {
    Grib2ScanFlagsDescriptor flags;
    flags.do_adjacent_rows_alternate = buf[0] & (1 << 4);
    flags.first_row_is_south = buf[0] & (1 << 6);
    flags.first_col_is_west = !(buf[0] & (1 << 7));
    flags.ary_is_column_major = buf[0] & (1 << 5);

    return flags;
}

Grib2LatLonProjectionDescriptor Grib2LatLonProjectionDescriptor::from_buffer(const g2int* buf) {
    // Should take into account the basic angle division from the file at some point. For now, just assume 1e-6.
    const float angle_unit = 1e-6;

    return {
        buf[11] * angle_unit,
        buf[12] * angle_unit,
        buf[14] * angle_unit,
        buf[15] * angle_unit,
        buf[16] * angle_unit,
        buf[17] * angle_unit
    };
}

Grib2LambertProjectionDescriptor Grib2LambertProjectionDescriptor::from_buffer(const g2int* buf) {
    const float angle_unit = 1e-6;
    const float spacing_unit = 1e-3;

    return {
        buf[0] * angle_unit,
        buf[1] * angle_unit,
        buf[3] * angle_unit,
        buf[4] * angle_unit,
        buf[9] * angle_unit,
        buf[10] * angle_unit,
        buf[5] * spacing_unit,
        buf[6] * spacing_unit
    };
}


NS_PROJ::operation::CoordinateTransformerNNPtr Grib2GridDef::get_fwd_transform(PJ_CONTEXT* ctx) const {
    auto crs = this->get_crs();
    auto base_crs = crs->baseCRS();

    return make_transformer(base_crs, crs, ctx);
}

NS_PROJ::operation::CoordinateTransformerNNPtr Grib2GridDef::get_inv_transform(PJ_CONTEXT* ctx) const {
    auto crs = this->get_crs();
    auto base_crs = crs->baseCRS();

    return make_transformer(crs, base_crs, ctx);
}


#define GRIB2_GRID_DEFINITION_CASE(name) \
    case name::template_number: \
        return std::make_shared<name>(name::from_buffer(template_buf));

std::shared_ptr<Grib2GridDef> zaphod::select_grid_def_template(g2int template_num, g2int* template_buf) {
    switch (template_num) {
        GRIB2_GRID_DEFINITION_CASE(Grib2GridDefLatLon)
        GRIB2_GRID_DEFINITION_CASE(Grib2GridDefLambert)
        default:
            throw "Unknown grid template number";
    }
}


NS_PROJ::crs::ProjectedCRSNNPtr Grib2GridDefLatLon::get_crs() const {
    NS_PROJ::util::PropertyMap props;
    props.set("name", "");

    // This is hard coded to convert to a cartesian space, but for lat/lon projections, I don't actually want to do this.
    auto conversion = NS_PROJ::operation::Conversion::createEquidistantCylindrical(props,
        NS_PROJ::common::Angle(0),
        NS_PROJ::common::Angle(0),
        NS_PROJ::common::Length(0),
        NS_PROJ::common::Length(0)
    );

    const auto earth_shape = std::get<Grib2EarthShapeDescriptor>(this->descriptors);

    auto crs = make_proj_crs(earth_shape.get_proj_ellipsoid(), conversion);
    return NN_CHECK_ASSERT(NS_PROJ::util::nn_dynamic_pointer_cast<NS_PROJ::crs::ProjectedCRS>(crs));
}

std::map<std::string, float> Grib2GridDefLatLon::get_proj_parameters() const {
    const auto earth_shape = std::get<Grib2EarthShapeDescriptor>(this->descriptors);

    std::map<std::string, float> params;
    params["a"] = earth_shape.earth_semimajor;
    params["b"] = earth_shape.earth_semiminor;
    return params;
}

std::vector<float> Grib2GridDefLatLon::get_xs() const {
    const auto grid = std::get<Grib2SpatialGridDescriptor>(this->descriptors);
    const auto scan_flags = std::get<Grib2ScanFlagsDescriptor>(this->descriptors);
    const auto projection = std::get<Grib2LatLonProjectionDescriptor>(this->descriptors);
    
    std::vector<float> xs(grid.ni);
    const float inc_sign = scan_flags.first_col_is_west ? 1 : -1;

    for (size_t i = 0; i < grid.ni; i++) {
        xs[i] = projection.longitude_first + inc_sign * i * projection.di;
    }

    return xs;
}

std::vector<float> Grib2GridDefLatLon::get_ys() const {
    const auto grid = std::get<Grib2SpatialGridDescriptor>(this->descriptors);
    const auto scan_flags = std::get<Grib2ScanFlagsDescriptor>(this->descriptors);
    const auto projection = std::get<Grib2LatLonProjectionDescriptor>(this->descriptors);

    std::vector<float> ys(grid.nj);
    const float inc_sign = scan_flags.first_row_is_south ? 1 : -1;

    for (size_t j = 0; j < grid.nj; j++) {
        ys[j] = projection.latitude_first + inc_sign * j * projection.dj;
    }

    return ys;
}

std::tuple<std::vector<float>, std::vector<float>> Grib2GridDefLatLon::get_lonlats() const {
    const auto grid = std::get<Grib2SpatialGridDescriptor>(this->descriptors);
    const auto scan_flags = std::get<Grib2ScanFlagsDescriptor>(this->descriptors);
    const auto projection = std::get<Grib2LatLonProjectionDescriptor>(this->descriptors);

    std::vector<float> lons(grid.ni * grid.nj);
    std::vector<float> lats(grid.ni * grid.nj);

    const float inc_sign_lon = scan_flags.first_col_is_west ? 1 : -1;
    const float inc_sign_lat = scan_flags.first_row_is_south ? 1 : -1;

    for (size_t j = 0; j < grid.nj; j++) {
        float lat = projection.latitude_first + inc_sign_lat * j * projection.dj;

        for (size_t i = 0; i < grid.ni; i++) {
            float lon = projection.longitude_first + inc_sign_lon * i * projection.di;

            size_t idx = i + grid.ni * j;
            lons[idx] = lon;
            lats[idx] = lat;
        } 
    }

    return std::tuple<std::vector<float>, std::vector<float>>(lons, lats);
}


NS_PROJ::crs::ProjectedCRSNNPtr Grib2GridDefLambert::get_crs() const {
    const auto projection = std::get<Grib2LambertProjectionDescriptor>(this->descriptors);
    const auto earth_shape = std::get<Grib2EarthShapeDescriptor>(this->descriptors);

    NS_PROJ::util::PropertyMap props;
    props.set("name", "");

    auto conversion = NS_PROJ::operation::Conversion::createLambertConicConformal_2SP(props,
        NS_PROJ::common::Angle(projection.center_latitude),
        NS_PROJ::common::Angle(projection.standard_longitude),
        NS_PROJ::common::Angle(projection.standard_latitude_1),
        NS_PROJ::common::Angle(projection.standard_latitude_2),
        NS_PROJ::common::Length(0),
        NS_PROJ::common::Length(0)
    );

    auto crs = make_proj_crs(earth_shape.get_proj_ellipsoid(), conversion);
    return NN_CHECK_ASSERT(NS_PROJ::util::nn_dynamic_pointer_cast<NS_PROJ::crs::ProjectedCRS>(crs));
}

std::map<std::string, float> Grib2GridDefLambert::get_proj_parameters() const {
    const auto projection = std::get<Grib2LambertProjectionDescriptor>(this->descriptors);
    const auto earth_shape = std::get<Grib2EarthShapeDescriptor>(this->descriptors);

    std::map<std::string, float> params;
    params["lon_0"] = projection.standard_longitude;
    params["lat_0"] = projection.center_latitude;
    params["lat_1"] = projection.standard_latitude_1;
    params["lat_2"] = projection.standard_latitude_2;
    params["a"] = earth_shape.earth_semimajor;
    params["b"] = earth_shape.earth_semiminor;
    return params;
}

// Should get_xs() and get_ys() be (largely) implemented in the base class?
std::vector<float> Grib2GridDefLambert::get_xs() const {
    const auto grid = std::get<Grib2SpatialGridDescriptor>(this->descriptors);
    const auto scan_flags = std::get<Grib2ScanFlagsDescriptor>(this->descriptors);
    const auto projection = std::get<Grib2LambertProjectionDescriptor>(this->descriptors);

    PJ_CONTEXT *ctx = proj_context_create();
    auto trans = this->get_fwd_transform(ctx);

    float x_first, y_first;
    std::tie(x_first, y_first) = transform_point(projection.longitude_first, projection.latitude_first, trans);

    std::vector<float> xs(grid.ni);
    const float inc_sign = scan_flags.first_col_is_west ? 1 : -1;

    for (size_t i = 0; i < grid.ni; i++) {
        xs[i] = x_first + inc_sign * i * projection.di;
    }

    proj_context_destroy(ctx);

    return xs;
}

std::vector<float> Grib2GridDefLambert::get_ys() const {
    const auto grid = std::get<Grib2SpatialGridDescriptor>(this->descriptors);
    const auto scan_flags = std::get<Grib2ScanFlagsDescriptor>(this->descriptors);
    const auto projection = std::get<Grib2LambertProjectionDescriptor>(this->descriptors);

    PJ_CONTEXT *ctx = proj_context_create();
    auto trans = this->get_fwd_transform(ctx);

    float x_first, y_first;
    std::tie(x_first, y_first) = transform_point(projection.longitude_first, projection.latitude_first, trans);

    std::vector<float> ys(grid.nj);
    const float inc_sign = scan_flags.first_row_is_south ? 1 : -1;

    for (size_t j = 0; j < grid.nj; j++) {
        ys[j] = y_first + inc_sign * j * projection.dj;
    }

    proj_context_destroy(ctx);

    return ys;
}

std::tuple<std::vector<float>, std::vector<float>> Grib2GridDefLambert::get_lonlats() const {
    const auto grid = std::get<Grib2SpatialGridDescriptor>(this->descriptors);
    const auto scan_flags = std::get<Grib2ScanFlagsDescriptor>(this->descriptors);
    const auto projection = std::get<Grib2LambertProjectionDescriptor>(this->descriptors);

    PJ_CONTEXT *ctx = proj_context_create();
    auto trans_fwd = this->get_fwd_transform(ctx);
    auto trans_inv = this->get_inv_transform(ctx);

    std::vector<float> lons(grid.ni * grid.nj);
    std::vector<float> lats(grid.ni * grid.nj);

    float x_first, y_first;
    std::tie(x_first, y_first) = transform_point(projection.longitude_first, projection.latitude_first, trans_fwd);

    const float inc_sign_i = scan_flags.first_col_is_west ? 1 : -1;
    const float inc_sign_j = scan_flags.first_row_is_south ? 1 : -1;

    for (size_t j = 0; j < grid.nj; j++) {
        float y = y_first + inc_sign_j * j * projection.dj;

        for (size_t i = 0; i < grid.ni; i++) {
            float x = x_first + inc_sign_i * i * projection.di;

            size_t idx = i + grid.ni * j;
            std::tie(lons[idx], lats[idx]) = transform_point(x, y, trans_inv);
        } 
    }

    proj_context_destroy(ctx);

    return std::tuple<std::vector<float>, std::vector<float>>(lons, lats);
}