
extern "C" {
    #include <grib2.h>
}

#include <chrono>

#include "grib2_key.h"
#include "grib2_templates.h"

using namespace std::chrono_literals;


float value_from_buffer(const g2int* buf) {
    return pow(10, -buf[0]) * buf[1];
}

struct Grib2ParameterDescriptor {
    g2int parameter_category;
    g2int parameter_number;
    g2int type_of_generating_process;

    static Grib2ParameterDescriptor from_buffer(const g2int* buf);
};

Grib2ParameterDescriptor Grib2ParameterDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1], buf[2]};
}

struct Grib2ProbabilityDescriptor {
    g2int prob_number;
    g2int number_of_probs;
    g2int prob_type;
    float prob_lower_limit;
    float prob_upper_limit;

    static Grib2ProbabilityDescriptor from_buffer(const g2int* buf);
};

Grib2ProbabilityDescriptor Grib2ProbabilityDescriptor::from_buffer(const g2int* buf) {
    return {
        buf[0],
        buf[1],
        buf[2],
        value_from_buffer(buf + 3),
        value_from_buffer(buf + 5)
    };
}

struct Grib2ProcessIdDescriptor {
    g2int background_process_id;
    g2int process_id;

    static Grib2ProcessIdDescriptor from_buffer(const g2int* buf);
};

Grib2ProcessIdDescriptor Grib2ProcessIdDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1]};
}

struct Grib2ForecastTimeDescriptor {
    g2int data_cutoff_hours;
    g2int data_cutoff_minutes;
    std::chrono::duration<unsigned int> forecast_time;
    
    static Grib2ForecastTimeDescriptor from_buffer(const g2int* buf);
};

Grib2ForecastTimeDescriptor Grib2ForecastTimeDescriptor::from_buffer(const g2int* buf) {
    unsigned int units = buf[2];
    int fcst_time_raw = buf[3];

    std::chrono::duration<unsigned int> fcst_time;

    switch(units) {
        case 0: fcst_time = fcst_time_raw * 1min; break;
        case 1: fcst_time = fcst_time_raw * 1h; break;
        case 2: fcst_time = fcst_time_raw * 24h; break;
        case 13: fcst_time = fcst_time_raw * 1s; break;
        default: throw "Unhandled time units";
    }

    return {buf[0], buf[1], fcst_time};
}

struct Grib2LayerDescriptor {
    g2int surface_1_type;
    float surface_1_value;
    g2int surface_2_type;
    float surface_2_value;

    static Grib2LayerDescriptor from_buffer(const g2int* buf);
};

Grib2LayerDescriptor Grib2LayerDescriptor::from_buffer(const g2int* buf) {
    return {
        buf[0],
        value_from_buffer(buf + 1),
        buf[3],
        value_from_buffer(buf + 4)
    };
}

struct ProductKey {
    template <size_t N, typename... Descrs>
    static Grib2Key get_key(const Grib2Template<N, ProductKey, Descrs...>& templ);
};

// WTF is this .template syntax, C++?
#define GET_FROM_DESCRIPTOR(descr_type, param_type, param_name) \
    constexpr auto get_##param_name = [](const descr_type& descr)->Grib2KeyVal<param_type> { return descr.param_name; }; \
    const auto param_name = templ.template do_with_descriptor<descr_type>(get_##param_name, Grib2KeyVal<param_type>(Grib2KeyNotPresent()));

template <size_t N, typename... Descrs>
Grib2Key ProductKey::get_key(const Grib2Template<N, ProductKey, Descrs...>& templ) {
    GET_FROM_DESCRIPTOR(Grib2ParameterDescriptor, g2int, parameter_category)
    GET_FROM_DESCRIPTOR(Grib2ParameterDescriptor, g2int, parameter_number)
    GET_FROM_DESCRIPTOR(Grib2LayerDescriptor, g2int, surface_1_type)
    GET_FROM_DESCRIPTOR(Grib2LayerDescriptor, float, surface_1_value)
    GET_FROM_DESCRIPTOR(Grib2LayerDescriptor, g2int, surface_2_type)
    GET_FROM_DESCRIPTOR(Grib2LayerDescriptor, float, surface_2_value)

    return Grib2Key(
        Grib2KeyIgnore(),
        g2int(N),
        parameter_category,
        parameter_number,
        surface_1_type,
        surface_1_value,
        surface_2_type,
        surface_2_value
    );
}

using Grib2ProductAnaFcst = Grib2Template<0, ProductKey, Grib2Descriptor<0, Grib2ParameterDescriptor>,
                                                         Grib2Descriptor<3, Grib2ProcessIdDescriptor>,
                                                         Grib2Descriptor<5, Grib2ForecastTimeDescriptor>,
                                                         Grib2Descriptor<9, Grib2LayerDescriptor>>;