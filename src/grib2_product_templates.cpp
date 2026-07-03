
#include "grib2_product_templates.h"

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