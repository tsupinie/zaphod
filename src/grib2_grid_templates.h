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

#include "grib2_templates.h"

struct Grib2EarthShapeDescriptor {
    Grib2EarthShapeDescriptor() = delete;
    Grib2EarthShapeDescriptor(float semimajor, float semiminor, bool is_sphere) : 
        earth_semimajor(semimajor), earth_semiminor(semiminor), earth_is_sphere(is_sphere) {}

    float earth_semimajor;
    float earth_semiminor;
    bool earth_is_sphere;

    static Grib2EarthShapeDescriptor from_buffer(const g2int* buf);
    NS_PROJ::datum::EllipsoidNNPtr get_proj_ellipsoid() const;
};

struct Grib2SpatialGridDescriptor {
    unsigned int ni;
    unsigned int nj;

    static Grib2SpatialGridDescriptor from_buffer(const g2int* buf);
};

struct Grib2ScanFlagsDescriptor {
    bool do_adjacent_rows_alternate;
    bool first_row_is_south;
    bool first_col_is_west;
    bool ary_is_column_major;

    static Grib2ScanFlagsDescriptor from_buffer(const g2int* buf);
};

struct Grib2LatLonProjectionDescriptor {
    float latitude_first;
    float longitude_first;
    float latitude_last;
    float longitude_last;
    float di;
    float dj;

    static Grib2LatLonProjectionDescriptor from_buffer(const g2int* buf);
};

struct Grib2LambertProjectionDescriptor {
    float center_latitude;
    float standard_longitude;
    float standard_latitude_1;
    float standard_latitude_2;
    float latitude_first;
    float longitude_first;
    float di;
    float dj;

    static Grib2LambertProjectionDescriptor from_buffer(const g2int* buf);
};

struct Grib2GridDef {
    virtual std::string get_proj_type() const = 0;
    virtual std::map<std::string, float> get_proj_parameters() const = 0;

    
    virtual std::tuple<std::vector<float>, std::vector<float>> get_lonlats() const = 0;
    virtual std::vector<float> get_xs() const = 0;
    virtual std::vector<float> get_ys() const = 0;

    NS_PROJ::operation::CoordinateTransformerNNPtr get_fwd_transform(PJ_CONTEXT* ctx) const;
    NS_PROJ::operation::CoordinateTransformerNNPtr get_inv_transform(PJ_CONTEXT* ctx) const;

    protected:
    virtual NS_PROJ::crs::ProjectedCRSNNPtr get_crs() const = 0;
};

std::shared_ptr<Grib2GridDef> select_grid_def_template(g2int template_num, g2int* template_buf);

#define GRIB2_GRID_TEMPLATE(name, type) \
    struct name : public name##Base, public Grib2GridDef { \
        name(const name##Base& base) : name##Base(base) {} \
        name(const name##Base::DescriptorTupleType& descriptors) : name##Base(descriptors) {} \
        \
        static name from_buffer(const g2int* buf) { return name##Base::from_buffer(buf); }; \
        \
        std::map<std::string, float> get_proj_parameters() const; \
        std::string get_proj_type() const { return type; }; \
        std::tuple<std::vector<float>, std::vector<float>> get_lonlats() const; \
        std::vector<float> get_xs() const; \
        std::vector<float> get_ys() const; \
        \
        protected: \
        NS_PROJ::crs::ProjectedCRSNNPtr get_crs() const; \
    };

using Grib2GridDefLatLonBase = Grib2Template<0, Grib2Descriptor<0, Grib2EarthShapeDescriptor>,
                                                Grib2Descriptor<7, Grib2SpatialGridDescriptor>,
                                                Grib2Descriptor<9, Grib2LatLonProjectionDescriptor>,
                                                Grib2Descriptor<18, Grib2ScanFlagsDescriptor>>;

GRIB2_GRID_TEMPLATE(Grib2GridDefLatLon, "latlon")

using Grib2GridDefLambertBase = Grib2Template<30, Grib2Descriptor<0, Grib2EarthShapeDescriptor>,
                                                  Grib2Descriptor<7, Grib2SpatialGridDescriptor>,
                                                  Grib2Descriptor<9, Grib2LambertProjectionDescriptor>,
                                                  Grib2Descriptor<18, Grib2ScanFlagsDescriptor>>;

GRIB2_GRID_TEMPLATE(Grib2GridDefLambert, "lcc")

std::tuple<float, float> transform_point(float x_in, float y_in, const NS_PROJ::operation::CoordinateTransformerNNPtr& trans);
NS_PROJ::operation::CoordinateTransformerNNPtr make_transformer(NS_PROJ::crs::CRSNNPtr crs_from, NS_PROJ::crs::CRSNNPtr crs_to, PJ_CONTEXT* ctx);

#endif