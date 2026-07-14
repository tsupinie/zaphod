
#include <zaphod/key.h>
#include <zaphod/table_defs.h>

using namespace zaphod;

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2KeyIgnore& key) {
    stream << "<ignore>";
    return stream;
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2KeyNotPresent& key) {
    stream << "<not present>";
    return stream;
}

Grib2Key Grib2Key::and_disc_cat_param(const std::string& discipline, const std::string& category, const std::string& param) {
    auto disc_table = g2_tables.get_table(0, 0);
    auto disc_entry = disc_table.get_entry(discipline);

    auto cat_table = g2_tables.get_table(4, 1, disc_entry.number);
    auto cat_entry = cat_table.get_entry(category);

    auto param_table = g2_tables.get_table(4, 2, disc_entry.number, cat_entry.number);
    auto param_entry = param_table.get_entry(param);

    Grib2Key g2key(*this);
    g2key.discipline = disc_entry.number;
    g2key.param_category = cat_entry.number;
    g2key.param_number = param_entry.number;
    return g2key;
}

Grib2Key Grib2Key::and_level_1_type(const std::string& level_1_type) {
    auto table = g2_tables.get_table(4, 5);
    auto entry = table.get_entry(level_1_type);

    Grib2Key g2key(*this);
    g2key.level_1_type = entry.number;
    return g2key;
}

Grib2Key Grib2Key::and_level_2_type(const std::string& level_2_type) {
    auto table = g2_tables.get_table(4, 5);
    auto entry = table.get_entry(level_2_type);

    Grib2Key g2key(*this);
    g2key.level_2_type = entry.number;
    return g2key;
}

bool Grib2Key::operator==(const Grib2Key& other) const {
    const bool discipline_matches = Grib2Key::param_equals(this->discipline, other.discipline);
    const bool pdt_number_matches = Grib2Key::param_equals(this->pdt_number, other.pdt_number);
    const bool param_cat_matches = Grib2Key::param_equals(this->param_category, other.param_category);
    const bool param_num_matches = Grib2Key::param_equals(this->param_number, other.param_number);
    const bool surface_1_matches = Grib2Key::param_equals(this->level_1_type, other.level_1_type);
    const bool level_1_matches = Grib2Key::param_equals(this->level_1, other.level_1);
    const bool surface_2_matches = Grib2Key::param_equals(this->level_2_type, other.level_2_type);
    const bool level_2_matches = Grib2Key::param_equals(this->level_2, other.level_2);

    return discipline_matches && pdt_number_matches && param_cat_matches && param_num_matches &&
        surface_1_matches && level_1_matches && surface_2_matches && level_2_matches;
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2Key& key) {
    stream << "Grib2Key { discipline=" << key.discipline << ", " <<
                            "pdt_num=" << key.pdt_number << ", " <<
                          "param_cat=" << key.param_category << ", " <<
                          "param_num=" << key.param_number << ", " <<
                         "level1type=" << key.level_1_type << ", " <<
                             "level1=" << key.level_1 << ", " <<
                         "level2type=" << key.level_2_type << ", " <<
                             "level2=" << key.level_2 << " }";
    return stream;
}