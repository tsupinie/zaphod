
#include <iostream>
#include <vector>

#include <zaphod.h>

#include "utils.h"

using namespace zaphod;

int main() {
    const std::string fname = "data/24042802.rap.t02z.awp130bgrbf00.grib2";

    try {
        Grib2File g2f = Grib2File::scan_file(fname);

        std::cout << g2f;

        Grib2Key key = Grib2Key::with_disc_cat_param("Meteorology", "Temperature", "Temperature")
                                .and_level_1_type("Height AGL")
                                .and_level_1(2.);

        std::vector<Grib2Field> fields = g2f.get_fields({key});
        std::vector<float> data = fields[0].get_data();
        
        std::cout << data << std::endl;

    }
    catch (std::string exc) {
        std::cout << exc << std::endl;
        return 1;
    }
    catch (const char* exc) {
        std::cout << exc << std::endl;
        return 1;
    }

    return 0;
}