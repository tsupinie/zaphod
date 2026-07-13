#ifndef __ZAPHOD_GRIB2_READER__
#define __ZAPHOD_GRIB2_READER__

extern "C" {
    #include <grib2.h>
}

#include <filesystem>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <iostream>

#include <cmath>

#include "grid_templates.h"
#include "key.h"
#include "product_templates.h"

namespace zaphod {

class Grib2Field {
    public:
    Grib2Field(std::vector<char> buffer, g2int ifld);
    Grib2Field(const Grib2Field& other) : field{other.field}, product_def(other.product_def), grid_def{other.grid_def}, buffer(other.buffer), i_field(other.i_field) {}
    std::vector<float> get_data();

    bool matches(const std::vector<Grib2Key>& keys) const;
    std::string noaa_abbreviation() const;
    std::chrono::system_clock::time_point init_datetime() const;
    std::chrono::system_clock::duration fcst_time() const;
    std::chrono::system_clock::time_point valid_datetime() const;

    std::map<std::string, float> get_proj_parameters() const { return this->grid_def->get_proj_parameters(); };
    std::string get_proj_type() const { return this->grid_def->get_proj_type(); };
    g2int get_ni() const { return this->grid_def->get_ni(); };
    g2int get_nj() const { return this->grid_def->get_nj(); };
    std::vector<float> get_xs() const { return this->grid_def->get_xs(); };
    std::vector<float> get_ys() const { return this->grid_def->get_ys(); };
    std::tuple<std::vector<float>, std::vector<float>> get_lonlats() const { return this->grid_def->get_lonlats(); };
    std::string get_product_summary_string() const { return this->product_def->get_summary_string(this->field->discipline); };
    
    friend std::ostream& operator<<(std::ostream& stream, const Grib2Field& field);

    private:
    std::shared_ptr<gribfield> field;
    std::shared_ptr<Grib2ProductDef> product_def;
    std::shared_ptr<Grib2GridDef> grid_def;
    g2int i_field;
    std::vector<char> buffer;
};

std::ostream& operator<<(std::ostream& stream, const Grib2Field& field);

class Grib2File {
    public:
    Grib2File() {}
    Grib2File(const Grib2File& other) : field_list(other.field_list) {}
    
    static Grib2File scan_file(const std::string& fname);

    std::vector<Grib2Field> get_fields(const std::vector<Grib2Key>& keys);
    std::vector<Grib2Field> get_fields() { return this->field_list; };

    friend std::ostream& operator<<(std::ostream& stream, const Grib2File& g2f);
    
    private:
    Grib2File(std::vector<Grib2Field> field_list) : field_list(field_list) {}
    
    std::vector<Grib2Field> field_list;
};

std::ostream& operator<<(std::ostream& stream, const Grib2File& g2f);

};

#endif