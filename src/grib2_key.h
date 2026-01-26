#ifndef __GRIB2SPC_GRIB2KEY__
#define __GRIB2SPC_GRIB2KEY__

#include <variant>
#include <iostream>

#include <cmath>

#include "grib2_grid_templates.h"

template <class T, class... Types>
std::ostream& operator<<(std::ostream& stream, const std::variant<T, Types...>& var) {
    std::visit([&stream](auto&& elem) { stream << elem; }, var);
    return stream;
}

struct Grib2KeyIgnore {
    constexpr bool operator==(const Grib2KeyIgnore& other) const { return true; }
};

std::ostream& operator<<(std::ostream& stream, const Grib2KeyIgnore& key) {
    stream << "<ignore>";
    return stream;
}

struct Grib2KeyNotPresent {
    constexpr bool operator==(const Grib2KeyNotPresent& other) const { return true; }
};

std::ostream& operator<<(std::ostream& stream, const Grib2KeyNotPresent& key) {
    stream << "<not present>";
    return stream;
}

template <typename T>
using Grib2KeyVal = std::variant<Grib2KeyIgnore, Grib2KeyNotPresent, T>;

#define KEY_VARIABLE(type, name) \
    Grib2KeyVal<type> name; \
    static Grib2Key with_##name(type name) { return Grib2Key().and_##name(name); } \
    Grib2Key and_##name(type name) { Grib2Key g2key(*this); g2key.name = name; return g2key; }

struct Grib2Key {
    Grib2Key() : discipline(Grib2KeyIgnore()), pdt_number(Grib2KeyIgnore()), param_category(Grib2KeyIgnore()), param_number(Grib2KeyIgnore()),
                 fixed_surface_1(Grib2KeyIgnore()), level_1(Grib2KeyIgnore()), fixed_surface_2(Grib2KeyIgnore()), level_2(Grib2KeyIgnore()) {};

    Grib2Key(const Grib2KeyVal<g2int> discipline, const Grib2KeyVal<g2int> pdt_number, 
             const Grib2KeyVal<g2int> param_category, const Grib2KeyVal<g2int> param_number,
             const Grib2KeyVal<g2int> fixed_surface_1, const Grib2KeyVal<float> level_1, 
             const Grib2KeyVal<g2int> fixed_surface_2, const Grib2KeyVal<float> level_2) : 
                 discipline(discipline), pdt_number(pdt_number), param_category(param_category), param_number(param_number),
                 fixed_surface_1(fixed_surface_1), level_1(level_1), fixed_surface_2(fixed_surface_2), level_2(level_2) {}

    KEY_VARIABLE(g2int, discipline)
    KEY_VARIABLE(g2int, pdt_number)
    KEY_VARIABLE(g2int, param_category)
    KEY_VARIABLE(g2int, param_number)
    KEY_VARIABLE(g2int, fixed_surface_1)
    KEY_VARIABLE(float, level_1)
    KEY_VARIABLE(g2int, fixed_surface_2)
    KEY_VARIABLE(float, level_2)

    bool operator==(const Grib2Key& other) const;

    private:
    template<class T>
    static constexpr bool should_ignore(const Grib2KeyVal<T>& val) {
        return std::holds_alternative<Grib2KeyIgnore>(val);
    }

    template<class T>
    static inline bool param_equals(const Grib2KeyVal<T>& val1, const Grib2KeyVal<T>& val2) {
        return Grib2Key::should_ignore(val1) || Grib2Key::should_ignore(val2) || val1 == val2;
    }
};

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

std::ostream& operator<<(std::ostream& stream, const Grib2Key& key) {
    stream << "Grib2Key { discipline=" << key.discipline << ", " <<
                            "pdt_num=" << key.pdt_number << ", " <<
                          "param_cat=" << key.param_category << ", " <<
                          "param_num=" << key.param_number << ", " <<
                           "surface1=" << key.fixed_surface_1 << ", " <<
                             "level1=" << key.level_1 << ", " <<
                           "surface2=" << key.fixed_surface_2 << ", " <<
                             "level2=" << key.level_2 << " }";
    return stream;
}

#endif