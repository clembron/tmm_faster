#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/numpy.h>
#include <omp.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "TMMCore.h"

namespace py = pybind11;

/**
 * Utility to convert degrees to radians.
 */
inline double deg2rad(double deg) {
    return deg * (TMMCore::PI / 180.0);
}

PYBIND11_MODULE(tmm_faster_cpp, m) {
    m.doc() = "Ultra-fast TMM module: Zero-copy pointer integration and OpenMP loop fusion";
    m.attr("__version__") = TMMCore::VERSION;

    // 1. Bind the Result Structure
    py::class_<TMMResult>(m, "result")
        .def_readwrite("R_avg", &TMMResult::R_avg)
        .def_readwrite("T_avg", &TMMResult::T_avg)
        .def_readwrite("R_s", &TMMResult::R_s)
        .def_readwrite("R_p", &TMMResult::R_p)
        .def_readwrite("T_s", &TMMResult::T_s)
        .def_readwrite("T_p", &TMMResult::T_p);

    // 2. Bind the Core Class
    py::class_<TMMCore>(m, "core")
        .def(py::init<>())
        .def_property_readonly("version", [](const TMMCore&) { return TMMCore::VERSION; })

        // Bulk Calculation: Coherent (Optimized for 2D NumPy Arrays)
        .def("calc_coherent", [](TMMCore& self,
            py::array_t<std::complex<double>> n_matrix,
            const std::vector<double>& d,
            const std::vector<double>& angles_deg,
            const std::vector<double>& wavelengths) {

                auto n_proxy = n_matrix.unchecked<2>(); 
                size_t n_wl = n_matrix.shape(0);
                size_t n_layers = n_matrix.shape(1);
                size_t n_ang = angles_deg.size();

                if (n_wl != wavelengths.size() || n_layers != d.size()) 
                    throw std::runtime_error("Dimension mismatch: n_matrix must match wavelengths and d-vector.");

                // Pre-convert angles
                std::vector<double> angles_rad(n_ang);
                for (size_t j = 0; j < n_ang; ++j) {
                    if (std::abs(angles_deg[j]) > 90.0) {
                        throw std::domain_error("Angle out of range: Absolute value of angle must be <= 90 degrees.");
                    }
                    angles_rad[j] = deg2rad(angles_deg[j]);
                }

                // Allocate NumPy outputs
                auto R_avg = py::array_t<double>({ n_wl, n_ang });
                auto T_avg = py::array_t<double>({ n_wl, n_ang });
                auto R_s = py::array_t<double>({ n_wl, n_ang });
                auto R_p = py::array_t<double>({ n_wl, n_ang });
                auto T_s = py::array_t<double>({ n_wl, n_ang });
                auto T_p = py::array_t<double>({ n_wl, n_ang });

                auto r_ptr = R_avg.mutable_unchecked<2>();
                auto t_ptr = T_avg.mutable_unchecked<2>();
                auto rs_ptr = R_s.mutable_unchecked<2>();
                auto rp_ptr = R_p.mutable_unchecked<2>();
                auto ts_ptr = T_s.mutable_unchecked<2>();
                auto tp_ptr = T_p.mutable_unchecked<2>();
                
                auto compute_tmm = [&](int i, int j) {
                    const cdbl* n_row_ptr = reinterpret_cast<const cdbl*>(&n_proxy(i, 0));
                    TMMResult res = self.calculate_coherent_ptr(n_row_ptr, d.data(), n_layers, angles_rad[j], wavelengths[i]);
                    r_ptr(i, j) = res.R_avg;   t_ptr(i, j) = res.T_avg;
                    rs_ptr(i, j) = res.R_s;    rp_ptr(i, j) = res.R_p;
                    ts_ptr(i, j) = res.T_s;    tp_ptr(i, j) = res.T_p;
                };

                {
                    py::gil_scoped_release release;

                    if (n_wl >= n_ang) {
                        #pragma omp parallel for schedule(static)
                        for (int i = 0; i < static_cast<int>(n_wl); ++i) {
                            for (int j = 0; j < static_cast<int>(n_ang); ++j) {
                                compute_tmm(i, j);
                            }
                        }
                    } else {
                        for (int i = 0; i < static_cast<int>(n_wl); ++i) {
                            #pragma omp parallel for schedule(static)
                            for (int j = 0; j < static_cast<int>(n_ang); ++j) {
                                compute_tmm(i, j);
                            }
                        }
                    }
                }

                py::dict results;
                results["R_avg"] = R_avg; results["T_avg"] = T_avg;
                results["R_s"] = R_s;     results["R_p"] = R_p;
                results["T_s"] = T_s;     results["T_p"] = T_p;
                return results;

            }, py::arg("n"), py::arg("d"), py::arg("angles"), py::arg("wavelengths"))

        // Bulk Calculation: Incoherent
        .def("calc_incoherent", [](TMMCore& self,
            py::array_t<std::complex<double>> n_matrix,
            const std::vector<double>& d,
            const std::vector<char>& c,
            const std::vector<double>& angles_deg,
            const std::vector<double>& wavelengths) {

                auto n_proxy = n_matrix.unchecked<2>(); 
                size_t n_wl = n_matrix.shape(0);
                size_t n_layers = n_matrix.shape(1);
                size_t n_ang = angles_deg.size();

                if (n_wl != wavelengths.size() || n_layers != d.size() || n_layers != c.size()) 
                    throw std::runtime_error("Dimension mismatch in incoherent setup.");

                std::vector<double> angles_rad(n_ang);
                for (size_t j = 0; j < n_ang; ++j) {
                    if (std::abs(angles_deg[j]) > 90.0) {
                        throw std::domain_error("Angle out of range: Absolute value of angle must be <= 90 degrees.");
                    }
                    angles_rad[j] = deg2rad(angles_deg[j]);
                }

                auto R_avg = py::array_t<double>({ n_wl, n_ang });
                auto T_avg = py::array_t<double>({ n_wl, n_ang });
                auto R_s = py::array_t<double>({ n_wl, n_ang });
                auto R_p = py::array_t<double>({ n_wl, n_ang });
                auto T_s = py::array_t<double>({ n_wl, n_ang });
                auto T_p = py::array_t<double>({ n_wl, n_ang });

                auto r_ptr = R_avg.mutable_unchecked<2>();
                auto t_ptr = T_avg.mutable_unchecked<2>();
                auto rs_ptr = R_s.mutable_unchecked<2>();
                auto rp_ptr = R_p.mutable_unchecked<2>();
                auto ts_ptr = T_s.mutable_unchecked<2>();
                auto tp_ptr = T_p.mutable_unchecked<2>();
                
                auto compute_incoherent_tmm = [&](int i, int j) {
                    const cdbl* n_row_ptr = reinterpret_cast<const cdbl*>(&n_proxy(i, 0));
                    TMMResult res = self.calculate_incoherent_ptr(n_row_ptr, d.data(), c.data(), n_layers, angles_rad[j], wavelengths[i]);
                    r_ptr(i, j) = res.R_avg;   t_ptr(i, j) = res.T_avg;
                    rs_ptr(i, j) = res.R_s;    rp_ptr(i, j) = res.R_p;
                    ts_ptr(i, j) = res.T_s;    tp_ptr(i, j) = res.T_p;
                };

                {
                    py::gil_scoped_release release;
                    if (n_wl >= n_ang) {
                        #pragma omp parallel for schedule(static)
                        for (int i = 0; i < static_cast<int>(n_wl); ++i) {
                            for (int j = 0; j < static_cast<int>(n_ang); ++j) {
                                compute_incoherent_tmm(i, j);
                            }
                        }
                    } else {
                        for (int i = 0; i < static_cast<int>(n_wl); ++i) {
                            #pragma omp parallel for schedule(static)
                            for (int j = 0; j < static_cast<int>(n_ang); ++j) {
                                compute_incoherent_tmm(i, j);
                            }
                        }
                    }
                }

                py::dict results;
                results["R_avg"] = R_avg; results["T_avg"] = T_avg;
                results["R_s"] = R_s;     results["R_p"] = R_p;
                results["T_s"] = T_s;     results["T_p"] = T_p;
                return results;

            }, py::arg("n"), py::arg("d"), py::arg("c"), py::arg("angles"), py::arg("wavelengths"));
}
