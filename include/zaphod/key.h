#ifndef __ZAPHOD_GRIB2_KEY__
#define __ZAPHOD_GRIB2_KEY__

#include <variant>
#include <iostream>
#include <string>
#include <chrono>

#include <cmath>

#include "grid_templates.h"

namespace zaphod {

template <class T, class... Types>
std::ostream& operator<<(std::ostream& stream, const std::variant<T, Types...>& var) {
    std::visit([&stream](auto&& elem) { stream << elem; }, var);
    return stream;
}

struct Grib2KeyIgnore {
    constexpr bool operator==(const Grib2KeyIgnore& other) const { return true; }
};

std::ostream& operator<<(std::ostream& stream, const Grib2KeyIgnore& key);

struct Grib2KeyNotPresent {
    constexpr bool operator==(const Grib2KeyNotPresent& other) const { return true; }
};

std::ostream& operator<<(std::ostream& stream, const Grib2KeyNotPresent& key);

template <typename T>
using Grib2KeyVal = std::variant<Grib2KeyIgnore, Grib2KeyNotPresent, T>;

template <typename O, typename I, typename F>
Grib2KeyVal<O> cast_key_val(Grib2KeyVal<I> input, F&& func) {
    if (std::holds_alternative<Grib2KeyNotPresent>(input)) {
        return Grib2KeyVal<O>(Grib2KeyNotPresent());
    }
    if (std::holds_alternative<Grib2KeyIgnore>(input)) {
        return Grib2KeyVal<O>(Grib2KeyIgnore());
    }

    auto f = std::function(std::forward<F>(func));
    return Grib2KeyVal<O>(f(std::get<I>(input)));
}

#define KEY_VARIABLE(type, name) \
    Grib2KeyVal<type> name; \
    static Grib2Key with_##name(type name) { return Grib2Key().and_##name(name); } \
    Grib2Key and_##name(type name) { Grib2Key g2key(*this); g2key.name = name; return g2key; }

#define KEY_VARIABLE_STRING(name) \
    static Grib2Key with_##name(const std::string& name) { return Grib2Key().and_##name(name); } \
    Grib2Key and_##name(const std::string& name);

struct Grib2Key {
    Grib2Key() : discipline(Grib2KeyIgnore()), pdt_number(Grib2KeyIgnore()), param_category(Grib2KeyIgnore()), param_number(Grib2KeyIgnore()),
                 level_1_type(Grib2KeyIgnore()), level_1(Grib2KeyIgnore()), level_2_type(Grib2KeyIgnore()), level_2(Grib2KeyIgnore()) {};

    Grib2Key(const Grib2KeyVal<g2int> discipline, const Grib2KeyVal<g2int> pdt_number, 
             const Grib2KeyVal<g2int> param_category, const Grib2KeyVal<g2int> param_number,
             const Grib2KeyVal<g2int> level_1_type, const Grib2KeyVal<float> level_1, 
             const Grib2KeyVal<g2int> level_2_type, const Grib2KeyVal<float> level_2,
             const Grib2KeyVal<std::chrono::system_clock::duration> agg_length) : 
                 discipline(discipline), pdt_number(pdt_number), param_category(param_category), param_number(param_number),
                 level_1_type(level_1_type), level_1(level_1), level_2_type(level_2_type), level_2(level_2),
                 agg_length(agg_length) {}

    KEY_VARIABLE(g2int, discipline)
    KEY_VARIABLE(g2int, pdt_number)
    KEY_VARIABLE(g2int, param_category)
    KEY_VARIABLE(g2int, param_number)
    KEY_VARIABLE(g2int, level_1_type)
    KEY_VARIABLE(float, level_1)
    KEY_VARIABLE(g2int, level_2_type)
    KEY_VARIABLE(float, level_2)
    KEY_VARIABLE(std::chrono::system_clock::duration, agg_length)

    static Grib2Key with_disc_cat_param(const std::string& discipline, const std::string& category, const std::string& param) { 
        return Grib2Key().and_disc_cat_param(discipline, category, param);
    }
    Grib2Key and_disc_cat_param(const std::string& discipline, const std::string& category, const std::string& param);

    static Grib2Key with_disc_cat(const std::string& discipline, const std::string& category) { 
        return Grib2Key().and_disc_cat(discipline, category);
    }
    Grib2Key and_disc_cat(const std::string& discipline, const std::string& category);

    KEY_VARIABLE_STRING(discipline)
    KEY_VARIABLE_STRING(level_1_type)
    KEY_VARIABLE_STRING(level_2_type)

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

std::ostream& operator<<(std::ostream& stream, const Grib2Key& key);

}

#endif