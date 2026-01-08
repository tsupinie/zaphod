
#include "grib2_key.h"

bool Grib2Key::operator==(const Grib2Key& other) const {
    const bool discipline_matches = Grib2Key::param_equals(this->discipline, other.discipline);
    const bool pdt_number_matches = Grib2Key::param_equals(this->pdt_number, other.pdt_number);
    const bool param_cat_matches = Grib2Key::param_equals(this->param_category, other.param_category);
    const bool param_num_matches = Grib2Key::param_equals(this->param_number, other.param_number);
    const bool surface_1_matches = Grib2Key::param_equals(this->fixed_surface_1, other.fixed_surface_1);
    const bool level_1_matches = Grib2Key::param_equals(this->level_1, other.level_1);
    const bool surface_2_matches = Grib2Key::param_equals(this->fixed_surface_2, other.fixed_surface_2);
    const bool level_2_matches = Grib2Key::param_equals(this->level_2, other.level_2);

    return discipline_matches && pdt_number_matches && param_cat_matches && param_num_matches &&
        surface_1_matches && level_1_matches && surface_2_matches && level_2_matches;
}