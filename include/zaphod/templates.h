#ifndef __ZAPHOD_GRIB2_TEMPLATES__
#define __ZAPHOD_GRIB2_TEMPLATES__

extern "C" {
    #include <grib2.h>
}

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <functional>
#include <iostream>

#include "key.h"

namespace zaphod {

template <typename T, typename... Tuple>
struct tuple_has_type;

template <typename T>
struct tuple_has_type<T, std::tuple<>> : std::false_type {};

template <typename T, typename U, typename... Ts>
struct tuple_has_type<T, std::tuple<U, Ts...>> : tuple_has_type<T, std::tuple<Ts...>> {};

template <typename T, typename...Ts>
struct tuple_has_type<T, std::tuple<T, Ts...>> : std::true_type {};

template <size_t Offset, typename Descr>
struct Grib2Descriptor {
    typedef Descr type;
    static Descr from_buffer(const g2int* buf) { return Descr::from_buffer(buf + Offset); }
};

template <typename T, typename F, typename R, typename... Tuple_t>
constexpr std::enable_if_t<tuple_has_type<T, std::tuple<Tuple_t...>>::value, R> tuple_do_with_element(std::tuple<Tuple_t...> tup, F&& func, R default_value) {
    auto f = std::function(std::forward<F>(func));
    T val = std::get<T>(tup);
    return f(val);
}

template <typename T, typename F, typename R, typename... Tuple_t>
constexpr std::enable_if_t<!tuple_has_type<T, std::tuple<Tuple_t...>>::value, R> tuple_do_with_element(std::tuple<Tuple_t...> tup, F&& func, R default_value) {
    return default_value;
}

template <size_t N, typename... Descrs>
struct Grib2Template {
    static constexpr size_t template_number = N;
    typedef std::tuple<typename Descrs::type...> DescriptorTupleType;

    Grib2Template(const DescriptorTupleType& descriptors) : descriptors(descriptors) {}
    Grib2Template(const Grib2Template<N, Descrs...>& other) : descriptors(other.descriptors) {}

    DescriptorTupleType descriptors;

    static Grib2Template<N, Descrs...> from_buffer(const g2int* buf);
    
    // template <typename Descr>
    // constexpr bool has();

    template <typename Descr, typename Func, typename RetVal>
    constexpr RetVal do_with_descriptor(Func&& func, RetVal default_value) const {
        return tuple_do_with_element<Descr>(this->descriptors, func, default_value);
    }
};

template <size_t N, typename... Descrs>
Grib2Template<N, Descrs...> Grib2Template<N, Descrs...>::from_buffer(const g2int* buf) {
    auto tup = std::make_tuple(Descrs::from_buffer(buf)...);
    return Grib2Template<N, Descrs...>(tup);
}

struct Grib2TemplateNotPresent {
    constexpr bool operator==(const Grib2TemplateNotPresent& other) const { return true; }
};

template <typename T>
using Grib2TemplateVal = std::variant<Grib2TemplateNotPresent, T>;

// WTF is this .template syntax, C++?
#define GRIB2_TEMPLATE_GET_DESCRIPTOR_VARIANT(descr_type, param_name) \
    typedef decltype(descr_type::param_name) ValueType_##param_name; \
    using TemplateVal_##param_name = Grib2TemplateVal<ValueType_##param_name>; \
    constexpr auto get_##param_name = [](const descr_type& descr)->TemplateVal_##param_name { return descr.param_name; }; \
    const auto templ_##param_name = templ.template do_with_descriptor<descr_type>(get_##param_name, TemplateVal_##param_name(Grib2TemplateNotPresent()));

#define GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR_REQUIRED(descr_type, param_name, error) \
    GRIB2_TEMPLATE_GET_DESCRIPTOR_VARIANT(descr_type, param_name) \
    \
    if (std::holds_alternative<Grib2TemplateNotPresent>(templ_##param_name)) { \
        throw error; \
    } \
    \
    param_name = *std::get_if<ValueType_##param_name>(&templ_##param_name);

#define GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR_DEFAULT(descr_type, param_name, default_val) \
    GRIB2_TEMPLATE_GET_DESCRIPTOR_VARIANT(descr_type, param_name) \
    \
    if (std::holds_alternative<Grib2TemplateNotPresent>(templ_##param_name)) { \
        param_name = default_val; \
    }\
    \
    param_name = *std::get_if<ValueType_##param_name>(&templ_##param_name);

#define GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR(descr_type, param_name) \
    GRIB2_TEMPLATE_GET_DESCRIPTOR_VARIANT(descr_type, param_name) \
    \
    if (const ValueType_##param_name *maybe_##param_name = std::get_if<ValueType_##param_name>(&templ_##param_name)) { \
        param_name = *maybe_##param_name; \
    }\

}

#endif