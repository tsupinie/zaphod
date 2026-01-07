
extern "C" {
    #include <grib2.h>
}

#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

#include <climits>
#include <cstring>
#include <ctime>

#define WORD 32 /**< Number of bits in four bytes. */
#include <arpa/inet.h> /* ntohl() function for Unix/Mac. */

#include "grib2_reader.h"

/** Byte swap 64-bit ints. This converts native-endian 8-byte ints into
 * big-endian 8-byte ints. */
#define hton64(y) (((uint64_t)htonl(y)) << WORD | htonl(y >> WORD))

using namespace std::chrono_literals;
namespace fs = std::filesystem;

#define G2_EXPAND 1

std::chrono::system_clock::time_point time_point_from_buffer(g2int* buf) {
    std::tm t{};

    t.tm_year = buf[0] - 1900;
    t.tm_mon = buf[1] - 1;
    t.tm_mday = buf[2];
    t.tm_hour = buf[3];
    t.tm_min = buf[4];
    t.tm_sec = buf[5];
    t.tm_isdst = -1; // Well, that's a few hours I'll never get back

    return std::chrono::system_clock::from_time_t(std::mktime(&t));
}

bool Grib2Key::operator==(const Grib2Key& other) const {
    const bool discipline_matches = Grib2Key::param_equals(this->discipline, other.discipline);
    const bool pdt_number_matches = Grib2Key::param_equals(this->pdt_number, other.pdt_number);
    const bool param_cat_matches = Grib2Key::param_equals(this->param_category, other.param_category);
    const bool param_num_matches = Grib2Key::param_equals(this->param_number, other.param_number);
    const bool surface_1_matches = Grib2Key::param_equals(this->fixed_surface_1, other.fixed_surface_1);
    const bool level_1_matches = Grib2Key::param_equals(this->level_1, other.level_1);
    const bool surface_2_matches = Grib2Key::param_equals(this->fixed_surface_2, other.fixed_surface_2);
    const bool level_2_matches = Grib2Key::param_equals(this->level_2, other.level_2);

    return discipline_matches && pdt_number_matches && param_cat_matches && param_num_matches &&
        surface_1_matches && level_1_matches && surface_2_matches && level_2_matches;
}

Grib2Field::Grib2Field(std::vector<char> buffer, g2int ifld) {
    unsigned char *buffer_msg = reinterpret_cast<unsigned char*>(buffer.data());

    gribfield* fld;
    g2_getfld(buffer_msg, ifld + 1, 0, G2_EXPAND, &fld);

    this->field = std::shared_ptr<gribfield>(fld, g2_free);
    this->buffer = buffer;
    this->i_field = ifld;

    this->grid_def = Grib2GridDef::select_grid_def_template(this->field->igdtnum, this->field->igdtmpl);
}

bool Grib2Field::matches(const std::vector<Grib2Key>& keys) const {
    float level_1 = pow(10, -this->field->ipdtmpl[10]) * this->field->ipdtmpl[11];
    float level_2 = pow(10, -this->field->ipdtmpl[13]) * this->field->ipdtmpl[14];

    const Grib2Key key_to_match(
        this->field->discipline,
        this->field->ipdtnum,
        this->field->ipdtmpl[0],
        this->field->ipdtmpl[1],
        this->field->ipdtmpl[9],
        level_1,
        this->field->ipdtmpl[12],
        level_2
    );

    for (auto it = keys.begin(); it != keys.end(); it++) {
        if (*it == key_to_match) return true;
    }

    return false;
}

std::vector<float> Grib2Field::get_data() {
    size_t ni = this->field->igdtmpl[7];
    size_t nj = this->field->igdtmpl[8];

    gribfield* fld;

    unsigned char *buffer_msg = reinterpret_cast<unsigned char*>(this->buffer.data());
    g2_getfld(buffer_msg, this->i_field + 1, 1, G2_EXPAND, &fld);

    std::vector<float> ary(nj * ni);

    for (size_t j = 0; j < nj; j++) {
        for (size_t i = 0; i < ni; i++) {
            // TAS: need to account for the do_adjacent_rows_alternate flag here. Do the REFS members use that flag?
            ary[i + ni * j] = fld->fld[i + ni * j];
        }
    }

    g2_free(fld);

    return ary;
}

std::string Grib2Field::noaa_abbreviation() const {
    char abbrev[G2C_MAX_NOAA_ABBREV_LEN];
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
    if (this->field->ipdtnum >= 8 && this->field->ipdtnum <= 14) {
        // XXX: This may result in a stack overflow for certain grib files
        auto init_time = this->init_datetime();
        auto valid_time = time_point_from_buffer(&this->field->ipdtmpl[18]);
        return valid_time - init_time;
    }

    int fcst_time = this->field->ipdtmpl[8];
    unsigned char units = this->field->ipdtmpl[7];

    switch(units) {
        case 0: return fcst_time * 1min;
        case 1: return fcst_time * 1h;
        case 2: return fcst_time * 24h;
        case 13: return fcst_time * 1s;
        default: throw "Unhandled time units";
    }
}

std::chrono::system_clock::time_point Grib2Field::valid_datetime() const {
    return this->init_datetime() + this->fcst_time();
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
