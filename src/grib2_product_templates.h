#ifndef __GRIB2SPC_GRIB2_PRODUCT_TEMPLATES__
#define __GRIB2SPC_GRIB2_PRODUCT_TEMPLATES__

extern "C" {
    #include <grib2.h>
}

#include <chrono>

#include "grib2_key.h"
#include "grib2_templates.h"
#include "grib2_utils.h"


struct Grib2ParameterDescriptor {
    g2int parameter_category;
    g2int parameter_number;
    g2int type_of_generating_process;

    static Grib2ParameterDescriptor from_buffer(const g2int* buf);
};

struct Grib2ProbabilityDescriptor {
    g2int prob_number;
    g2int number_of_probs;
    g2int prob_type;
    float prob_lower_limit;
    float prob_upper_limit;

    static Grib2ProbabilityDescriptor from_buffer(const g2int* buf);
};

struct Grib2ProcessIdDescriptor {
    g2int background_process_id;
    g2int process_id;

    static Grib2ProcessIdDescriptor from_buffer(const g2int* buf);
};

struct Grib2ForecastTimeDescriptor {
    g2int data_cutoff_hours;
    g2int data_cutoff_minutes;
    std::chrono::duration<unsigned int> forecast_time;
    
    static Grib2ForecastTimeDescriptor from_buffer(const g2int* buf);
};

struct Grib2LayerDescriptor {
    g2int surface_1_type;
    float surface_1_value;
    g2int surface_2_type;
    float surface_2_value;

    static Grib2LayerDescriptor from_buffer(const g2int* buf);
};

struct Grib2EnsembleMemberDescriptor {
    g2int ensemble_type;
    g2int perturbation_num;
    g2int number_of_members;

    static Grib2EnsembleMemberDescriptor from_buffer(const g2int* buf);
};

struct Grib2AggregationDescriptor {
    struct Spec {
        constexpr static size_t n_elems = 6;

        g2int statistical_process_type;
        g2int time_increment_type;
        std::chrono::duration<unsigned int> length_of_time_range;
        std::chrono::duration<unsigned int> length_of_time_increment;
    };

    std::chrono::system_clock::time_point end_of_interval;
    g2int number_of_specs;
    g2int number_of_missing_values;
    std::vector<Spec> specs;

    static Grib2AggregationDescriptor from_buffer(const g2int* buf);
};

struct Grib2ProductDef {
    virtual Grib2Key get_key() const = 0;

    template <size_t N, typename... Descrs>
    Grib2Key get_key(const Grib2Template<N, Descrs...>& templ) const;

    virtual std::chrono::duration<unsigned int> get_forecast_time() const = 0;

    template <size_t N, typename... Descrs>
    std::chrono::duration<unsigned int> get_forecast_time(const Grib2Template<N, Descrs...>& templ) const;
};

template <size_t N, typename... Descrs>
Grib2Key Grib2ProductDef::get_key(const Grib2Template<N, Descrs...>& templ) const {
    Grib2KeyVal<g2int> parameter_category;
    Grib2KeyVal<g2int> parameter_number;
    Grib2KeyVal<g2int> surface_1_type;
    Grib2KeyVal<float> surface_1_value;
    Grib2KeyVal<g2int> surface_2_type;
    Grib2KeyVal<float> surface_2_value;

    GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR_DEFAULT(Grib2ParameterDescriptor, parameter_category, Grib2KeyNotPresent())
    GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR_DEFAULT(Grib2ParameterDescriptor, parameter_number, Grib2KeyNotPresent())
    GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR_DEFAULT(Grib2LayerDescriptor, surface_1_type, Grib2KeyNotPresent())
    GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR_DEFAULT(Grib2LayerDescriptor, surface_1_value, Grib2KeyNotPresent())
    GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR_DEFAULT(Grib2LayerDescriptor, surface_2_type, Grib2KeyNotPresent())
    GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR_DEFAULT(Grib2LayerDescriptor, surface_2_value, Grib2KeyNotPresent())

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

template <size_t N, typename... Descrs>
std::chrono::duration<unsigned int> Grib2ProductDef::get_forecast_time(const Grib2Template<N, Descrs...>& templ) const {
    std::string err_msg = std::string("Forecast time not present in template ") + std::to_string(N);

    std::chrono::duration<unsigned int> forecast_time;
    GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR_REQUIRED(Grib2ForecastTimeDescriptor, forecast_time, err_msg);

    std::vector<Grib2AggregationDescriptor::Spec> specs;
    GRIB2_TEMPLATE_GET_FROM_DESCRIPTOR(Grib2AggregationDescriptor, specs);

    if (specs.size() > 0) {
        forecast_time += specs[0].length_of_time_range;
    }

    return forecast_time;
}

std::shared_ptr<Grib2ProductDef> select_product_def_template(g2int template_num, g2int* template_buf);

#define GRIB2_PRODUCT_TEMPLATE(name) \
    struct name : public name##Base, public Grib2ProductDef { \
        name(const name##Base& base) : name##Base(base) {} \
        Grib2Key get_key() const { return Grib2ProductDef::get_key(*this); }; \
        std::chrono::duration<unsigned int> get_forecast_time() const { return Grib2ProductDef::get_forecast_time(*this); }; \
        static name from_buffer(const g2int* buf) { return name##Base::from_buffer(buf); }; \
    };

using Grib2ProductAnaFcstBase = Grib2Template<0, Grib2Descriptor<0, Grib2ParameterDescriptor>,
                                                 Grib2Descriptor<3, Grib2ProcessIdDescriptor>,
                                                 Grib2Descriptor<5, Grib2ForecastTimeDescriptor>,
                                                 Grib2Descriptor<9, Grib2LayerDescriptor>>;

GRIB2_PRODUCT_TEMPLATE(Grib2ProductAnaFcst)

using Grib2ProductEnsMemberBase = Grib2Template<1, Grib2Descriptor<0, Grib2ParameterDescriptor>,
                                                   Grib2Descriptor<3, Grib2ProcessIdDescriptor>,
                                                   Grib2Descriptor<5, Grib2ForecastTimeDescriptor>,
                                                   Grib2Descriptor<9, Grib2LayerDescriptor>,
                                                   Grib2Descriptor<15, Grib2EnsembleMemberDescriptor>>;

GRIB2_PRODUCT_TEMPLATE(Grib2ProductEnsMember)

using Grib2ProductAggregationBase = Grib2Template<8, Grib2Descriptor<0, Grib2ParameterDescriptor>,
                                                     Grib2Descriptor<3, Grib2ProcessIdDescriptor>,
                                                     Grib2Descriptor<5, Grib2ForecastTimeDescriptor>,
                                                     Grib2Descriptor<9, Grib2LayerDescriptor>,
                                                     Grib2Descriptor<15, Grib2AggregationDescriptor>>;

GRIB2_PRODUCT_TEMPLATE(Grib2ProductAggregation)
#endif