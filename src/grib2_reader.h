#ifndef __SPCPOST_GRIB2__
#define __SPCPOST_GRIB2__

extern "C" {
    #include <grib2.h>
}

#include <filesystem>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>

#include <cmath>

#include "grib2_grid_templates.h"
#include "grib2_key.h"
#include "grib2_product_templates.h"

namespace fs = std::filesystem;

class Grib2Field {
    public:
    Grib2Field(std::vector<char> buffer, g2int ifld);
    Grib2Field(const Grib2Field& other) : field{other.field}, grid_def{other.grid_def}, buffer(other.buffer), i_field(other.i_field) {}
    std::vector<float> get_data();

    bool matches(const std::vector<Grib2Key>& keys) const;
    std::string noaa_abbreviation() const;
    std::chrono::system_clock::time_point init_datetime() const;
    std::chrono::system_clock::duration fcst_time() const;
    std::chrono::system_clock::time_point valid_datetime() const;

    std::map<std::string, float> get_proj_parameters() const { return this->grid_def->get_proj_parameters(); };
    std::string get_proj_type() const { return this->grid_def->get_proj_type(); };
    std::vector<float> get_xs() const { return this->grid_def->get_xs(); };
    std::vector<float> get_ys() const { return this->grid_def->get_ys(); };
    std::tuple<std::vector<float>, std::vector<float>> get_lonlats() const { return this->grid_def->get_lonlats(); };

    private:
    std::shared_ptr<gribfield> field;
    std::shared_ptr<Grib2ProductDef> product_def;
    std::shared_ptr<Grib2GridDef> grid_def;
    g2int i_field;
    std::vector<char> buffer;
};
    
    
class Grib2File {
    public:
    Grib2File(const Grib2File& other) : field_list(other.field_list) {}
    
    static Grib2File scan_file(const std::string& fname);

    std::vector<Grib2Field> get_fields(const std::vector<Grib2Key>& keys);
    
    private:
    Grib2File(std::vector<Grib2Field> field_list) : field_list(field_list) {}
    
    std::vector<Grib2Field> field_list;
};


#endif