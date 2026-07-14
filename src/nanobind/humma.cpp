
#include <array>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/string.h>
#include <zaphod.h>

namespace nb = nanobind;

template <size_t D>
using arr_out_nd_t = nb::ndarray<nb::numpy, float, nb::ndim<D>, nb::c_contig>;

using arr_out_1d_t = arr_out_nd_t<1>;
using arr_out_2d_t = arr_out_nd_t<2>;

template <size_t D>
arr_out_nd_t<D> make_array(const std::array<size_t, D>& sizes) {
    size_t size = 1;
    for (size_t i = 0; i < D; i++) {
        size *= sizes[i];
    }

    float *ary = new float[size];
    nb::capsule owner(ary, [](void *p) noexcept { delete[] (float*)p; });
    return arr_out_nd_t<D>(ary, D, sizes.data(), owner);
}

class PyGrib2Field : public zaphod::Grib2Field {
    public:
    PyGrib2Field(const Grib2Field& other) : zaphod::Grib2Field(other) {};

    arr_out_2d_t get_data() {
        size_t ni = this->get_ni();
        size_t nj = this->get_nj();
        arr_out_2d_t data_ary = make_array<2>({nj, ni});

        std::vector<float> data = zaphod::Grib2Field::get_data();

        for (size_t i = 0; i < ni; i++) {
            for (size_t j = 0; j < nj; nj++) {
                data_ary(j, i) = data[i + ni * j];
            }
        }

        return data_ary;
    }

    arr_out_1d_t get_xs() const {
        size_t ni = this->get_ni();
        arr_out_1d_t xs_ary = make_array<1>({ni});

        std::vector<float> xs = zaphod::Grib2Field::get_xs();

        for (size_t i = 0; i < ni; i++) {
            xs_ary(i) = xs[i];
        }

        return xs_ary;
    }

    arr_out_1d_t get_ys() const {
        size_t nj = this->get_nj();
        arr_out_1d_t ys_ary = make_array<1>({nj});

        std::vector<float> ys = zaphod::Grib2Field::get_ys();

        for (size_t j = 0; j < nj; j++) {
            ys_ary(j) = ys[j];
        }

        return ys_ary;
    }

    std::tuple<arr_out_2d_t, arr_out_2d_t> get_lonlats() const {
        size_t ni = this->get_ni();
        size_t nj = this->get_nj();
        arr_out_2d_t lons_ary = make_array<2>({nj, ni});
        arr_out_2d_t lats_ary = make_array<2>({nj, ni});

        std::vector<float> lons, lats; 
        std::tie(lons, lats) = zaphod::Grib2Field::get_lonlats();

        for (size_t i = 0; i < ni; i++) {
            for (size_t j = 0; j < nj; nj++) {
                lons_ary(j, i) = lons[i + ni * j];
                lats_ary(j, i) = lats[i + ni * j];
            }
        }

        return std::make_tuple(lons_ary, lats_ary);
    }
};

class PyGrib2File : public zaphod::Grib2File {
    public:
    std::vector<PyGrib2Field> get_fields(const std::vector<zaphod::Grib2Key>& keys) { return this->to_py_field_list(zaphod::Grib2File::get_fields(keys)); };
    std::vector<PyGrib2Field> get_fields() { return this->to_py_field_list(zaphod::Grib2File::get_fields()); };

    static PyGrib2File scan_file(const std::string& fname) { return zaphod::Grib2File::scan_file(fname); };

    private:
    PyGrib2File(const zaphod::Grib2File& other) : zaphod::Grib2File(other) {};

    std::vector<PyGrib2Field> to_py_field_list(const std::vector<zaphod::Grib2Field>& vec) {
        std::vector<PyGrib2Field> ret;

        for (auto it = vec.begin(); it != vec.end(); it++) {
            ret.push_back(*it);
        }

        return ret;
    }
};

#define BIND_KEY_VARIABLE(name, type) \
    .def_static("with_"#name, nb::overload_cast<type>(&zaphod::Grib2Key::with_##name), nb::arg(#name)) \
    .def("and_"#name, nb::overload_cast<type>(&zaphod::Grib2Key::and_##name), nb::arg(#name))

NB_MODULE(humma, m) {
    nb::class_<PyGrib2File>(m, "Grib2File")
        .def_static("scan_file", &PyGrib2File::scan_file, nb::arg("fname"))
        .def("get_fields", nb::overload_cast<const std::vector<zaphod::Grib2Key>&>(&PyGrib2File::get_fields), nb::arg("keys"))
        .def("get_fields", nb::overload_cast<>(&PyGrib2File::get_fields));

    nb::class_<PyGrib2Field>(m, "Grib2Field")
        .def("get_data", &PyGrib2Field::get_data)
        .def("noaa_abbreviation", &PyGrib2Field::noaa_abbreviation)
        .def("init_datetime", &PyGrib2Field::init_datetime)
        .def("fcst_time", &PyGrib2Field::fcst_time)
        .def("valid_datetime", &PyGrib2Field::valid_datetime)
        .def("get_proj_parameters", &PyGrib2Field::get_proj_parameters)
        .def("get_proj_type", &PyGrib2Field::get_proj_type)
        .def("get_xs", &PyGrib2Field::get_xs)
        .def("get_ys", &PyGrib2Field::get_ys)
        .def("get_lonlats", &PyGrib2Field::get_lonlats);

    nb::class_<zaphod::Grib2Key>(m, "Grib2Key")
        BIND_KEY_VARIABLE(discipline, g2int)
        .def_static("with_disc_cat_param", &zaphod::Grib2Key::with_disc_cat_param, nb::arg("discipline"), nb::arg("category"), nb::arg("param"))
        .def("and_disc_cat_param", &zaphod::Grib2Key::and_disc_cat_param, nb::arg("discipline"), nb::arg("category"), nb::arg("param"))
        BIND_KEY_VARIABLE(pdt_number, g2int)
        BIND_KEY_VARIABLE(param_category, g2int)
        BIND_KEY_VARIABLE(param_number, g2int)
        BIND_KEY_VARIABLE(level_1_type, g2int)
        BIND_KEY_VARIABLE(level_1_type, const std::string&)
        BIND_KEY_VARIABLE(level_1, float)
        BIND_KEY_VARIABLE(level_2_type, g2int)
        BIND_KEY_VARIABLE(level_2_type, const std::string&)
        BIND_KEY_VARIABLE(level_2, float);

    m.attr("__version__") = "0.1.0";
    m.attr("__version_tuple__") = std::make_tuple(0, 1, 0);
}