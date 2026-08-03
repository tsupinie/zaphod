
#include <iostream>
#include <vector>

#include <zaphod.h>

#include "utils.h"

using namespace zaphod;

void print_key(Grib2File& g2f, const Grib2Key& key) {
    std::vector<Grib2Field> fields = g2f.get_fields({key});
    std::vector<float> data = fields[0].get_data();

    std::cout << data << std::endl;
    std::cout << fields[0].get_data_units() << std::endl;
}

int main() {
    std::string fname = "data/24042802.rap.t02z.awp130bgrbf00.grib2";

    try {
        Grib2Key key;
        Grib2File g2f = Grib2File::scan_file(fname);

        std::cout << g2f;

        key = Grib2Key::with_disc_cat_param("Meteorology", "Temperature", "Temperature")
                       .and_level_1_type("Height AGL")
                       .and_level_1(2.);
    
        print_key(g2f, key);
    }
    catch (std::runtime_error exc) {
        std::cout << exc.what() << std::endl;
        return 1;
    }

    fname = "data/gec00.master.2026072800.f024.grib2";

    try {
        Grib2Key key;
        Grib2File g2f = Grib2File::scan_file(fname);

        std::cout << g2f;

        key = Grib2Key::with_abbrev("PRES");

        std::vector<Grib2Field> fields = g2f.get_fields({key});
        std::cout << fields[0].get_ys() << std::endl;
    }
    catch (std::runtime_error exc) {
        std::cout << exc.what() << std::endl;
        return 1;
    }

    fname = "data/rrfs_2026080312f001.grib2";

    try {
        Grib2Key key;
        Grib2File g2f = Grib2File::scan_file(fname);

        std::cout << g2f;
    }
    catch (std::runtime_error exc) {
        std::cout << exc.what() << std::endl;
        return 1;
    }

    return 0;
}