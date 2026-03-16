#ifndef TMMCORE_H
#define TMMCORE_H

#include <vector>
#include <complex>
#include <string>

typedef std::complex<double> cdbl;

struct TMMResult {
    double R_s, R_p, T_s, T_p, R_avg, T_avg;
};

struct GroupedLayers {
    size_t num_inc_layers;
    size_t num_stacks;
    std::vector<size_t> all_from_inc; // Indices of 'i' layers
    std::vector<int> stack_from_inc;  // Maps space between inc layers to stack index
    std::vector<std::vector<cdbl>> stack_n_list;
    std::vector<std::vector<double>> stack_d_list;
    std::vector<std::vector<size_t>> all_from_stack;
};

class TMMCore {
public:
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double EPSILON = 1e-14;
    static constexpr const char* VERSION = "0.1.0";

    TMMCore() = default;

    // --- High Performance Pointer Interfaces ---
    TMMResult calculate_coherent_ptr(const cdbl* n_list, const double* d_list, size_t num_layers, cdbl th_0, double lambda, bool reverse = false, bool is_partial_stack = false);
    
    TMMResult calculate_incoherent_ptr(const cdbl* n_list, const double* d_list, const char* c_list, size_t num_layers, double th_0, double lambda);

    // --- Legacy Vector Interfaces (Wrappers) ---
    TMMResult calculate_coherent(const std::vector<cdbl>& n, const std::vector<double>& d, cdbl th_0, double lambda, bool is_partial_stack = false);
    TMMResult calculate_incoherent(const std::vector<cdbl>& n, const std::vector<double>& d, const std::vector<char>& c, double th_0, double lambda);

private:
    cdbl get_forward_theta(cdbl n_i, cdbl n_f, cdbl th_i);
    GroupedLayers group_layers(const cdbl* n_list, const double* d_list, const char* c_list, size_t num_layers);
};

#endif