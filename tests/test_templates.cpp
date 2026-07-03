
#include <iostream>

#include <grib2_product_templates.h>

int main() {
    g2int buf[15] = {3, 5, 0,  0, 0,  0, 0, 1, 24,  100, 0, 50000, 255, 0, 0};

    const auto templ = Grib2ProductAnaFcst::from_buffer(buf);

    const Grib2Key key = templ.get_key();
    const auto test_key = Grib2Key::with_param_category(3);

    std::cout << key << std::endl;
    std::cout << test_key << std::endl;
    std::cout << (test_key == key) << std::endl;

    return 0;
}