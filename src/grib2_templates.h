
extern "C" {
    #include <grib2.h>
}

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <functional>
#include <iostream>

#include "grib2_key.h"

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

template <size_t N, typename Key, typename... Descrs>
struct Grib2Template {
    static constexpr size_t template_number = N;
    Grib2Template(std::tuple<typename Descrs::type...> descriptors) : descriptors(descriptors) {}
    Grib2Template(const Grib2Template<N, Key, Descrs...>& other) : descriptors(other.descriptors) {}

    std::tuple<typename Descrs::type...> descriptors;

    static Grib2Template<N, Key, Descrs...> from_buffer(const g2int* buf);
    
    // template <typename Descr>
    // constexpr bool has();

    template <typename Descr, typename Func, typename RetVal>
    constexpr RetVal do_with_descriptor(Func&& func, RetVal default_value) const {
        return tuple_do_with_element<Descr>(this->descriptors, func, default_value);
    }

    Grib2Key get_key() const { return Key::get_key(*this); }
};

template <size_t N, typename Key, typename... Descrs>
Grib2Template<N, Key, Descrs...> Grib2Template<N, Key, Descrs...>::from_buffer(const g2int* buf) {
    auto tup = std::make_tuple(Descrs::from_buffer(buf)...);
    return Grib2Template<N, Key, Descrs...>(tup);
}