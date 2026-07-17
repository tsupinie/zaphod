
#include <iostream>
#include <chrono>

#include <zaphod.h>

using namespace zaphod;
using namespace std::chrono_literals;

int main() {
    g2int buf[15] = {3, 5, 0,  0, 0,  0, 0, 1, 24,  100, 0, 50000, 255, 0, 0};

    const auto templ = Grib2ProductAnaFcst::from_buffer(buf);

    try {
        std::cout << templ.get_level() << std::endl;
    }
    catch (ParameterNotInMessage exc) {
        std::cout << "No Level" << std::endl;
    }

    try {
        std::cout << templ.get_layer() << std::endl;
    }
    catch (ParameterNotInMessage exc) {
        std::cout << "No Layer" << std::endl;
    }

    const Grib2Key key = templ.get_key();
    const auto test_key = Grib2Key::with_param_category(3).and_agg_length(1h);

    std::cout << key << std::endl;
    std::cout << test_key << std::endl;
    std::cout << (test_key == key) << std::endl;

    return 0;
}