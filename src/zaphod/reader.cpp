
extern "C" {
    #include <grib2.h>
}

#include <iomanip>
#include <fstream>
#include <filesystem>

#include <climits>
#include <cstring>
#include <ctime>

#define WORD 32 /**< Number of bits in four bytes. */
#include <arpa/inet.h> /* ntohl() function for Unix/Mac. */

#include <zaphod/reader.h>
#include <zaphod/utils.h>

/** Byte swap 64-bit ints. This converts native-endian 8-byte ints into
 * big-endian 8-byte ints. */
#define hton64(y) (((uint64_t)htonl(y)) << WORD | htonl(y >> WORD))

using namespace std::chrono_literals;
namespace fs = std::filesystem;
using namespace zaphod;

#define G2_EXPAND 1


Grib2Field::Grib2Field(std::vector<char> buffer, g2int ifld) {
    unsigned char *buffer_msg = reinterpret_cast<unsigned char*>(buffer.data());

    gribfield* fld;
    g2_getfld(buffer_msg, ifld + 1, 0, G2_EXPAND, &fld);

    this->field = std::shared_ptr<gribfield>(fld, g2_free);
    this->buffer = buffer;
    this->i_field = ifld;

    this->grid_def = select_grid_def_template(this->field->igdtnum, this->field->igdtmpl);
    this->product_def = select_product_def_template(this->field->ipdtnum, this->field->ipdtmpl);
}

bool Grib2Field::matches(const std::vector<Grib2Key>& keys) const {
    const Grib2Key key_to_match = this->product_def->get_key().and_discipline(this->field->discipline);

    for (auto it = keys.begin(); it != keys.end(); it++) {
        if (*it == key_to_match) return true;
    }

    return false;
}

std::vector<float> Grib2Field::get_data() {
    size_t ni = this->get_ni();
    size_t nj = this->get_nj();

    std::vector<float> ary(nj * ni);

    this->get_data(ary.data());

    return ary;
}

void Grib2Field::get_data(float* buf) {
    size_t ni = this->get_ni();
    size_t nj = this->get_nj();

    gribfield* fld;
    unsigned char *buffer_msg = reinterpret_cast<unsigned char*>(this->buffer.data());
    g2_getfld(buffer_msg, this->i_field + 1, 1, G2_EXPAND, &fld);

    for (size_t j = 0; j < nj; j++) {
        for (size_t i = 0; i < ni; i++) {
            // TAS: need to account for the do_adjacent_rows_alternate flag here. Do the REFS members use that flag?
            buf[i + ni * j] = fld->fld[i + ni * j];
        }
    }

    g2_free(fld);
}

std::string Grib2Field::noaa_abbreviation() const {
    char abbrev[G2C_MAX_NOAA_ABBREV_LEN] = {};
    g2c_param_abbrev(this->field->discipline, this->field->ipdtmpl[0], this->field->ipdtmpl[1], abbrev);
    return std::string(abbrev);
}

std::chrono::system_clock::time_point Grib2Field::init_datetime() const {
    auto ref_time = time_point_from_buffer(&this->field->idsect[5]);

    if (this->field->idsect[4] == 0 || this->field->idsect[4] == 1) {
        return ref_time;
    }

    return ref_time - this->fcst_time();
}

std::chrono::system_clock::duration Grib2Field::fcst_time() const {
    return this->product_def->get_forecast_time();
}

std::chrono::system_clock::time_point Grib2Field::valid_datetime() const {
    return this->init_datetime() + this->fcst_time();
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2Field& field) {
    char time_str[100] = {};
    auto init_dt = field.init_datetime();
    std::time_t init_dt_c = std::chrono::system_clock::to_time_t(init_dt);
    std::strftime(time_str, sizeof(time_str), "%Y%m%d%H", std::localtime(&init_dt_c));

    stream << "d=" << time_str << ":" << field.noaa_abbreviation() << ":" << field.get_product_summary_string();
    return stream;
}


Grib2File Grib2File::scan_file(const std::string& fname) {
    const size_t BLOCK_SIZE = 128 * 1024;

    fs::path file_path = fname;
    size_t file_size = fs::file_size(file_path);

    std::vector<char> buffer(file_size, 0); 
    char *buffer_raw = buffer.data();
    unsigned char *buffer_u = reinterpret_cast<unsigned char*>(buffer_raw);

    std::ifstream fin(fname, std::ios::binary | std::ios::in);

    size_t n_blocks = file_size / BLOCK_SIZE;
    if (file_size % BLOCK_SIZE) n_blocks++;

    for (size_t iblock = 0; iblock < n_blocks; iblock++) {
       fin.read(buffer_raw + iblock * BLOCK_SIZE, BLOCK_SIZE);
    }

    g2int* listsec0 = new g2int[G2C_SECTION0_LEN];
    g2int* listsec1 = new g2int[G2C_SECTION1_LEN];
    g2int numfields, numlocal;

    size_t msg_start = 0;
    g2int n_fields = 0;
    size_t n_msg = 0;

    while (msg_start < file_size) {
        unsigned char *buffer_msg = buffer_u + msg_start;
        size_t msg_size = hton64(reinterpret_cast<size_t&>(buffer_raw[msg_start + 8]));

        g2_info(buffer_msg, listsec0, listsec1, &numfields, &numlocal);
        n_fields += numfields;

        msg_start += msg_size;
        n_msg++;
    }

    std::vector<Grib2Field> field_list;

    msg_start = 0;
    size_t ifld_list = 0;

    // I probably don't need to loop over the array twice. Not that it'll matter that much.
    for (size_t imsg = 0; imsg < n_msg; imsg++) {
        unsigned char *buffer_msg_u = buffer_u + msg_start;
        size_t msg_size = hton64(reinterpret_cast<size_t&>(buffer_raw[msg_start + 8]));

        g2_info(buffer_msg_u, listsec0, listsec1, &numfields, &numlocal);

        std::vector<char> buffer_msg(buffer.begin() + msg_start, buffer.begin() + msg_start + msg_size);

        for (size_t ifld = 0; ifld < numfields; ifld++) {
            field_list.push_back(Grib2Field(buffer_msg, ifld));
            ifld_list++;
        }

        msg_start += msg_size;
    }

    delete[] listsec0;
    delete[] listsec1;

    return Grib2File(field_list);
}

std::vector<Grib2Field> Grib2File::get_fields(const std::vector<Grib2Key>& keys) {
    std::vector<Grib2Field> vec;

    for (auto it = this->field_list.begin(); it != this->field_list.end(); it++) {
        if (it->matches(keys)) {
            vec.push_back(*it);
        }
    }

    return vec;
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2File& g2f) {
    size_t imsg = 0;

    for (auto it = g2f.field_list.begin(); it != g2f.field_list.end(); it++) {
        std::cout << imsg << ":" << *it << std::endl;
        imsg++;
    }
    return stream;
}
