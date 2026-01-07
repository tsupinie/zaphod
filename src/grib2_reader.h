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

namespace fs = std::filesystem;

struct Grib2Key {
    static constexpr g2int IGNORE_INT = -1;
    static constexpr float IGNORE_FLOAT = NAN;

    Grib2Key(g2int discipline, g2int pdt_number, g2int param_category, g2int param_number,
             g2int fixed_surface_1, float level_1, g2int fixed_surface_2, float level_2) : 
                 discipline(discipline), pdt_number(pdt_number), param_category(param_category), param_number(param_number),
                 fixed_surface_1(fixed_surface_1), level_1(level_1), fixed_surface_2(fixed_surface_2), level_2(level_2) {}

    g2int discipline;
    g2int pdt_number;
    g2int param_category;
    g2int param_number;
    g2int fixed_surface_1;
    float level_1;
    g2int fixed_surface_2;
    float level_2;

    bool operator==(const Grib2Key& other) const;

    private:
    template<class T>
    static inline bool should_ignore(T val) {
        if constexpr (std::is_same_v<T, g2int>) {
            return val == Grib2Key::IGNORE_INT;
        }
        else if constexpr (std::is_same_v<T, float>) {
            return std::isnan(val);
        }
        else {
            static_assert(!sizeof(T*), "Unknown type in Grib2Key::should_ignore()");
        }
    }

    template<class T>
    static inline bool param_equals(T val1, T val2) {
        return Grib2Key::should_ignore(val1) || Grib2Key::should_ignore(val2) || val1 == val2;
    }
};

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

    private:
    std::shared_ptr<gribfield> field;
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