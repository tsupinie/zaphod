
#include "grib2_product_templates.h"

std::shared_ptr<Grib2ProductDef> select_product_def_template(g2int template_num, g2int* template_buf) {
    switch (template_num) {
        case 0:
            return std::make_shared<Grib2ProductAnaFcst>(Grib2ProductAnaFcst::from_buffer(template_buf));
        default:
            throw "Unknown grid template number";
    }
}