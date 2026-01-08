
#include <variant>

#include <cmath>

#include "grib2_grid_templates.h"

struct Grib2KeyIgnore : std::false_type {};

template <typename T>
using Grib2KeyVal = std::variant<Grib2KeyIgnore, T>;

#define KEY_VARIABLE(type, name) \
    Grib2KeyVal<type> name; \
    static Grib2Key with_##name(type name) { return Grib2Key().and_##name(name); } \
    Grib2Key and_##name(type name) { Grib2Key g2key(*this); g2key.name = name; return g2key; }

struct Grib2Key {
    Grib2Key() : discipline(Grib2KeyIgnore()), pdt_number(Grib2KeyIgnore()), param_category(Grib2KeyIgnore()), param_number(Grib2KeyIgnore()),
                 fixed_surface_1(Grib2KeyIgnore()), level_1(Grib2KeyIgnore()), fixed_surface_2(Grib2KeyIgnore()), level_2(Grib2KeyIgnore()) {};

    Grib2Key(const g2int discipline, const g2int pdt_number, const g2int param_category, const g2int param_number,
             const g2int fixed_surface_1, const float level_1, const g2int fixed_surface_2, const float level_2) : 
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
        return !std::holds_alternative<Grib2KeyIgnore>(val);
    }

    template<class T>
    static inline bool param_equals(const Grib2KeyVal<T>& val1, const Grib2KeyVal<T>& val2) {
        return Grib2Key::should_ignore(val1) || Grib2Key::should_ignore(val2) || val1 == val2;
    }
};