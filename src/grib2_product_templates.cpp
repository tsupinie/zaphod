
#include "grib2_product_templates.h"

Grib2ParameterDescriptor Grib2ParameterDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1], buf[2]};
}

Grib2ProbabilityDescriptor Grib2ProbabilityDescriptor::from_buffer(const g2int* buf) {
    return {
        buf[0],
        buf[1],
        buf[2],
        value_from_buffer(buf + 3),
        value_from_buffer(buf + 5)
    };
}


Grib2ProcessIdDescriptor Grib2ProcessIdDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1]};
}

Grib2ForecastTimeDescriptor Grib2ForecastTimeDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1], duration_from_buffer(buf + 2)};
}

Grib2LayerDescriptor Grib2LayerDescriptor::from_buffer(const g2int* buf) {
    return {
        buf[0],
        value_from_buffer(buf + 1),
        buf[3],
        value_from_buffer(buf + 4)
    };
}

Grib2EnsembleMemberDescriptor Grib2EnsembleMemberDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1], buf[2]};
}

Grib2AggregationDescriptor Grib2AggregationDescriptor::from_buffer(const g2int* buf) {
    g2int n_specs = buf[6];
    std::vector<Spec> specs;

    for (size_t i = 0; i < n_specs; i++) {
        specs.push_back({
            buf[8 + Spec::n_elems * i + 0],
            buf[8 + Spec::n_elems * i + 1],
            duration_from_buffer(buf + 8 + Spec::n_elems * i + 2),
            duration_from_buffer(buf + 8 + Spec::n_elems * i + 4)
        });
    }

    return {
        time_point_from_buffer(buf),
        n_specs,
        buf[7],
        specs
    };
}

#define GRIB2_PRODUCT_DEFINITION_CASE(name) \
    case name::template_number: \
        return std::make_shared<name>(name::from_buffer(template_buf));

std::shared_ptr<Grib2ProductDef> select_product_def_template(g2int template_num, g2int* template_buf) {
    switch (template_num) {
        GRIB2_PRODUCT_DEFINITION_CASE(Grib2ProductAnaFcst)
        GRIB2_PRODUCT_DEFINITION_CASE(Grib2ProductEnsMember)
        GRIB2_PRODUCT_DEFINITION_CASE(Grib2ProductAggregation)
        default:
            throw "Unknown grid template number";
    }
}