
#include <iostream>

#include <grib2_product_templates.h>

int main() {
    g2int buf[15] = {3, 5, 0,  0, 0,  0, 0, 1, 24,  100, 0, 50000, 255, 0, 0};

    auto templ = Grib2ProductAnaFcst::from_buffer(buf);
    constexpr auto get_parameter_category = [](const Grib2ParameterDescriptor& descr)->Grib2KeyVal<g2int> { return descr.parameter_category; };
    Grib2KeyVal<g2int> parameter_category = templ.do_with_descriptor<Grib2ParameterDescriptor>(get_parameter_category, Grib2KeyVal<g2int>(Grib2KeyNotPresent()));

    const Grib2Key key = templ.get_key();

    auto test_key = Grib2Key::with_param_category(3);
    std::cout << key << std::endl;
    std::cout << test_key << std::endl;
    std::cout << (test_key == key) << std::endl;

    return 0;
}