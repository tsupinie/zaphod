
#include <sstream>

#include <zaphod/product_templates.h>
#include <zaphod/table_defs.h>

using namespace zaphod;

Grib2ParameterDescriptor Grib2ParameterDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1], buf[2]};
}

Grib2ProbabilityDescriptor Grib2ProbabilityDescriptor::from_buffer(const g2int* buf) {
    return {
        buf[0],
        buf[1],
        buf[2],
        value_from_buffer(buf + 3),
        value_from_buffer(buf + 5)
    };
}


Grib2ProcessIdDescriptor Grib2ProcessIdDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1]};
}

Grib2ForecastTimeDescriptor Grib2ForecastTimeDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1], duration_from_buffer(buf + 2)};
}

Grib2LayerDescriptor Grib2LayerDescriptor::from_buffer(const g2int* buf) {
    return {
        buf[0],
        value_from_buffer(buf + 1),
        buf[3],
        value_from_buffer(buf + 4)
    };
}

std::string Grib2LayerDescriptor::get_summary_string() const {
    const auto level_table = g2_tables.get_table(4, 5);
    const auto surface_1 = level_table.get_entry(this->surface_1_type);
    const auto surface_2 = level_table.get_entry(this->surface_2_type);

    std::stringstream ss;

    if (surface_1.units != "" || this->surface_1_type == 104 || this->surface_1_type == 105 || this->surface_1_type == 111
        || this->surface_1_type == 113 || this->surface_1_type == 115 || this->surface_1_type == 118 || this->surface_1_type == 119
        || this->surface_1_type == 150 || this->surface_1_type == 151 || this->surface_1_type == 152) {
        ss << this->surface_1_value;

        if (this->surface_2_type != 255)
            ss << "-" << this->surface_2_value;

        ss << " ";
    }

    if (surface_1.units != "") {
        ss << surface_1.units << " ";
    }

    ss << surface_1.meaning;

    return ss.str();
}

bool Grib2LayerDescriptor::is_level() const {
    return this->surface_2_type == 255 || this->surface_2_type == this->surface_1_type && this->surface_2_value == this->surface_1_value;
}

Level Grib2LayerDescriptor::get_level() const {
    if (!this->is_level())
        throw ParameterNotInMessage("Product is not defined at particular level");

    const auto level_table = g2_tables.get_table(4, 5);
    const auto surface_1 = level_table.get_entry(this->surface_1_type);

    return {surface_1.meaning, this->surface_1_value};
}

Layer Grib2LayerDescriptor::get_layer() const {
    if (this->is_level())
        throw ParameterNotInMessage("Product is not defined over a layer");

    const auto level_table = g2_tables.get_table(4, 5);
    const auto surface_1 = level_table.get_entry(this->surface_1_type);
    const auto surface_2 = level_table.get_entry(this->surface_2_type);

    return {
        {surface_1.meaning, this->surface_1_value},
        {surface_2.meaning, this->surface_2_value}
    };
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Level& lev) {
    stream << "Level {" << lev.coordinate << ", " << std::to_string(lev.level) << "}";
    return stream;
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Layer& lyr) {
    stream << "Layer {" << lyr.bottom << ", " << lyr.top << "}";
    return stream;
}

Grib2EnsembleMemberDescriptor Grib2EnsembleMemberDescriptor::from_buffer(const g2int* buf) {
    return {buf[0], buf[1], buf[2]};
}

Grib2AggregationDescriptor Grib2AggregationDescriptor::from_buffer(const g2int* buf) {
    g2int n_specs = buf[6];
    std::vector<Spec> specs;

    for (size_t i = 0; i < n_specs; i++) {
        specs.push_back({
            buf[8 + Spec::n_elems * i + 0],
            buf[8 + Spec::n_elems * i + 1],
            duration_from_buffer(buf + 8 + Spec::n_elems * i + 2),
            duration_from_buffer(buf + 8 + Spec::n_elems * i + 4)
        });
    }

    return {
        time_point_from_buffer(buf),
        n_specs,
        buf[7],
        specs
    };
}

#define GRIB2_PRODUCT_DEFINITION_CASE(name) \
    case name::template_number: \
        return std::make_shared<name>(name::from_buffer(template_buf));

std::shared_ptr<Grib2ProductDef> zaphod::select_product_def_template(g2int template_num, g2int* template_buf) {
    switch (template_num) {
        GRIB2_PRODUCT_DEFINITION_CASE(Grib2ProductAnaFcst)
        GRIB2_PRODUCT_DEFINITION_CASE(Grib2ProductEnsMem)
        GRIB2_PRODUCT_DEFINITION_CASE(Grib2ProductProb)
        GRIB2_PRODUCT_DEFINITION_CASE(Grib2ProductAgg)
        GRIB2_PRODUCT_DEFINITION_CASE(Grib2ProductProbAgg)
        GRIB2_PRODUCT_DEFINITION_CASE(Grib2ProductEnsMemAgg)
        default:
            throw std::runtime_error("Unknown grid template number: " + std::to_string(template_num));
    }
}