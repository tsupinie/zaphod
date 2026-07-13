
#include <iostream>

#include <grib2_reader.h>

using namespace zaphod;

int main() {
    const std::string fname = "data/24042802.rap.t02z.awp130bgrbf00.grib2";

    try {
        Grib2File g2f = Grib2File::scan_file(fname);

        std::cout << g2f;
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