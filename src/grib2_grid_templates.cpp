
#include <cmath>
#include <iostream>

#include "grib2_grid_templates.h"


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

std::tuple<float, float> transform_point(float x_in, float y_in, NS_PROJ::crs::CRSNNPtr crs_from, NS_PROJ::crs::CRSNNPtr crs_to, PJ_CONTEXT* ctx) {
    auto factory = NS_PROJ::operation::CoordinateOperationFactory::create();
    auto op = factory->createOperation(crs_from, crs_to);

    PJ_COORD coord = {{x_in, y_in, 0, HUGE_VAL}};
    PJ_COORD coord_trans = op->coordinateTransformer(ctx)->transform(coord);

    return {coord_trans.xy.x, coord_trans.xy.y};
}


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

    return Grib2GridDefEarthShape(earth_semimajor, earth_semiminor, earth_is_sphere);
}

NS_PROJ::datum::EllipsoidNNPtr Grib2GridDefEarthShape::get_proj_ellipsoid() {
    NS_PROJ::util::PropertyMap props;
    props.set("name", "");

    return NS_PROJ::datum::Ellipsoid::createTwoAxis(
        props,
        NS_PROJ::common::Length(this->earth_semimajor, NS_PROJ::common::UnitOfMeasure::METRE), 
        NS_PROJ::common::Length(this->earth_semiminor, NS_PROJ::common::UnitOfMeasure::METRE)
    );
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
    // Should take into account the basic angle division from the file at some point. For now, just assume 1e-6.
    const float angle_unit = 1e-6;

    return Grib2GridDefLatLon(
        Grib2GridDefEarthShape::from_buffer(buf),
        Grib2GridDefSpatialGrid::from_buffer(buf + 7),
        Grib2GridDefScanFlags::from_buffer(buf + 18),
        buf[11] * angle_unit,
        buf[12] * angle_unit,
        buf[14] * angle_unit,
        buf[15] * angle_unit,
        buf[16] * angle_unit,
        buf[17] * angle_unit
    );
}

NS_PROJ::crs::ProjectedCRSNNPtr Grib2GridDefLatLon::get_crs() {
    NS_PROJ::util::PropertyMap props;
    props.set("name", "");

    auto conversion = NS_PROJ::operation::Conversion::createEquidistantCylindrical(props,
        NS_PROJ::common::Angle(0),
        NS_PROJ::common::Angle(0),
        NS_PROJ::common::Length(0),
        NS_PROJ::common::Length(0)
    );

    auto crs = make_proj_crs(this->earth_shape.get_proj_ellipsoid(), conversion);
    return NN_CHECK_ASSERT(NS_PROJ::util::nn_dynamic_pointer_cast<NS_PROJ::crs::ProjectedCRS>(crs));
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
    const float angle_unit = 1e-6;
    const float spacing_unit = 1e-3;

    return Grib2GridDefLambert(
        Grib2GridDefEarthShape::from_buffer(buf),
        Grib2GridDefSpatialGrid::from_buffer(buf + 7),
        Grib2GridDefScanFlags::from_buffer(buf + 17),
        buf[9] * angle_unit,
        buf[10] * angle_unit,
        buf[12] * angle_unit,
        buf[13] * angle_unit,
        buf[18] * angle_unit,
        buf[19] * angle_unit,
        buf[14] * spacing_unit,
        buf[15] * spacing_unit
    );
}

NS_PROJ::crs::ProjectedCRSNNPtr Grib2GridDefLambert::get_crs() {
    NS_PROJ::util::PropertyMap props;
    props.set("name", "");

    auto conversion = NS_PROJ::operation::Conversion::createLambertConicConformal_2SP(props,
        NS_PROJ::common::Angle(this->center_latitude),
        NS_PROJ::common::Angle(this->standard_longitude),
        NS_PROJ::common::Angle(this->standard_latitude_1),
        NS_PROJ::common::Angle(this->standard_latitude_2),
        NS_PROJ::common::Length(0),
        NS_PROJ::common::Length(0)
    );

    auto crs = make_proj_crs(this->earth_shape.get_proj_ellipsoid(), conversion);
    return NN_CHECK_ASSERT(NS_PROJ::util::nn_dynamic_pointer_cast<NS_PROJ::crs::ProjectedCRS>(crs));
}

std::map<std::string, float> Grib2GridDefLambert::get_proj_parameters() {
    std::map<std::string, float> params;
    params["lon_0"] = this->standard_longitude;
    params["lat_0"] = this->center_latitude;
    params["lat_1"] = this->standard_latitude_1;
    params["lat_2"] = this->standard_latitude_2;
    params["a"] = this->earth_shape.earth_semimajor;
    params["b"] = this->earth_shape.earth_semiminor;
    return params;
}

// Should get_xs() and get_ys() be (largely) implemented in the base class?
std::vector<float> Grib2GridDefLambert::get_xs() {
    auto crs = this->get_crs();
    auto base_crs = crs->baseCRS();

    PJ_CONTEXT *ctx = proj_context_create();

    float x_first, y_first;
    std::tie(x_first, y_first) = transform_point(this->longitude_first, this->latitude_first, base_crs, crs, ctx);

    std::vector<float> xs(this->grid.ni);
    const float inc_sign = this->scan_flags.first_col_is_west ? 1 : -1;

    for (size_t i = 0; i < this->grid.ni; i++) {
        xs[i] = x_first + inc_sign * i * this->di;
    }

    proj_context_destroy(ctx);

    return xs;
}

std::vector<float> Grib2GridDefLambert::get_ys() {
    auto crs = this->get_crs();
    auto base_crs = crs->baseCRS();

    PJ_CONTEXT *ctx = proj_context_create();

    float x_first, y_first;
    std::tie(x_first, y_first) = transform_point(this->longitude_first, this->latitude_first, base_crs, crs, ctx);

    std::vector<float> ys(this->grid.nj);
    const float inc_sign = this->scan_flags.first_row_is_south ? 1 : -1;

    for (size_t j = 0; j < this->grid.nj; j++) {
        ys[j] = y_first + inc_sign * j * this->dj;
    }

    proj_context_destroy(ctx);

    return ys;
}