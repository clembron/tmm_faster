#include "TMMCore.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

cdbl TMMCore::get_forward_theta(cdbl n_i, cdbl n_f, cdbl th_i) {
    cdbl sin_th_f = (n_i * std::sin(th_i)) / n_f;
    cdbl th_f_guess = std::asin(sin_th_f);
    cdbl ncostheta = n_f * std::cos(th_f_guess);

    bool is_forward = (ncostheta.imag() > 100 * EPSILON) ||
                      (std::abs(ncostheta.imag()) <= 100 * EPSILON && ncostheta.real() > 0);

    return is_forward ? th_f_guess : cdbl(PI) - th_f_guess;
}

TMMResult TMMCore::calculate_coherent_ptr(const cdbl* n_list, const double* d_list, size_t num_layers, cdbl th_0, double lambda, bool reverse, bool is_partial_stack) {
    if (num_layers < 2 && !is_partial_stack) throw std::invalid_argument("Coherent system needs >= 2 layers.");

    std::vector<cdbl> th_list(num_layers);
    size_t last_idx = num_layers - 1;
    size_t start_node = reverse ? last_idx : 0;
    th_list[start_node] = th_0;

    if (!reverse) {
        for (size_t i = 1; i < num_layers; ++i)
            th_list[i] = get_forward_theta(n_list[i - 1], n_list[i], th_list[i - 1]);
    } else {
        for (int i = (int)last_idx - 1; i >= 0; --i)
            th_list[i] = get_forward_theta(n_list[i + 1], n_list[i], th_list[i + 1]);
    }

    auto solve_pol = [&](bool p_pol) {
        cdbl M00(1, 0), M01(0, 0), M10(0, 0), M11(1, 0);
        for (size_t i = 0; i < last_idx; ++i) {
            size_t idx_curr = reverse ? (last_idx - i) : i;
            size_t idx_next = reverse ? (last_idx - i - 1) : (i + 1);

            cdbl ni = n_list[idx_curr], nf = n_list[idx_next];
            cdbl ci = std::cos(th_list[idx_curr]), cf = std::cos(th_list[idx_next]);
            cdbl r, t_inv;

            if (p_pol) {
                r = (nf * ci - ni * cf) / (nf * ci + ni * cf);
                t_inv = (nf * ci + ni * cf) / (2.0 * ni * ci);
            } else {
                r = (ni * ci - nf * cf) / (ni * ci + nf * cf);
                t_inv = (ni * ci + nf * cf) / (2.0 * ni * ci);
            }

            cdbl nM00 = (M00 + M01 * r) * t_inv;
            cdbl nM01 = (M00 * r + M01) * t_inv;
            cdbl nM10 = (M10 + M11 * r) * t_inv;
            cdbl nM11 = (M10 * r + M11) * t_inv;
            M00 = nM00; M01 = nM01; M10 = nM10; M11 = nM11;

            if (i < num_layers - 2) {
                cdbl n_mid = n_list[idx_next], c_mid = std::cos(th_list[idx_next]);
                cdbl delta = (2.0 * PI * n_mid * c_mid / lambda) * d_list[idx_next];
                if (delta.imag() > 35.0) delta = cdbl(delta.real(), 35.0);
                cdbl p_val = std::exp(cdbl(0, -1) * delta);
                cdbl p_inv = 1.0 / p_val;
                M00 *= p_val; M10 *= p_val; M01 *= p_inv; M11 *= p_inv;
            }
        }
        double R = std::norm(M10 / M00);
        double factor = std::real(n_list[reverse ? 0 : last_idx] * std::cos(th_list[reverse ? 0 : last_idx])) /
                        std::real(n_list[reverse ? last_idx : 0] * std::cos(th_list[reverse ? last_idx : 0]));
        return std::make_pair(R, factor / std::norm(M00));
    };

    auto s = solve_pol(false); auto p = solve_pol(true);
    return { s.first, p.first, s.second, p.second, (s.first + p.first) * 0.5, (s.second + p.second) * 0.5 };
}

// Wrappers for backward compatibility
TMMResult TMMCore::calculate_coherent(const std::vector<cdbl>& n, const std::vector<double>& d, cdbl th_0, double lambda, bool is_partial) {
    return calculate_coherent_ptr(n.data(), d.data(), n.size(), th_0, lambda, false, is_partial);
}

// Incoherent grouping logic (internal)
GroupedLayers TMMCore::group_layers(const cdbl* n_list, const double* d_list, const char* c_list, size_t num_layers) {
    GroupedLayers res;
    res.num_stacks = 0;
    for (size_t i = 0; i < num_layers; ++i) if (c_list[i] == 'i') res.all_from_inc.push_back(i);
    res.num_inc_layers = res.all_from_inc.size();
    res.stack_from_inc.assign(res.num_inc_layers - 1, -1);

    for (size_t i = 0; i < res.num_inc_layers - 1; ++i) {
        size_t idx_start = res.all_from_inc[i], idx_end = res.all_from_inc[i + 1];
        if (idx_end - idx_start > 1) {
            res.stack_from_inc[i] = (int)res.num_stacks;
            std::vector<cdbl> s_n; std::vector<double> s_d;
            for (size_t j = idx_start; j <= idx_end; ++j) { s_n.push_back(n_list[j]); s_d.push_back(d_list[j]); }
            res.stack_n_list.push_back(s_n); res.stack_d_list.push_back(s_d);
            res.num_stacks++;
        }
    }
    return res;
}

TMMResult TMMCore::calculate_incoherent_ptr(const cdbl* n_list, const double* d_list, const char* c_list, size_t num_layers, double th_0, double lambda) {
    auto groups = group_layers(n_list, d_list, c_list, num_layers);
    std::vector<cdbl> th_all(num_layers);
    th_all[0] = th_0;
    for (size_t i = 1; i < num_layers; ++i) th_all[i] = get_forward_theta(n_list[i - 1], n_list[i], th_all[i - 1]);

    std::vector<TMMResult> s_fwd, s_rev;
    for (size_t i = 0; i < groups.num_stacks; ++i) {
        cdbl th_s = th_all[groups.all_from_inc[i]]; // Angle at start of stack
        s_fwd.push_back(calculate_coherent(groups.stack_n_list[i], groups.stack_d_list[i], th_s, lambda, true));
        
        // Reverse calculation: Start with angle in the last layer of the stack
        cdbl th_f = get_forward_theta(groups.stack_n_list[i].front(), groups.stack_n_list[i].back(), th_s);
        s_rev.push_back(calculate_coherent_ptr(groups.stack_n_list[i].data(), groups.stack_d_list[i].data(), groups.stack_n_list[i].size(), th_f, lambda, true, true));
    }

    auto solve_inc = [&](int pol) {
        auto get_RT = [&](size_t i, double& r, double& t, double& rb, double& tb) {
            int s_idx = groups.stack_from_inc[i];
            if (s_idx == -1) {
                cdbl n1 = n_list[groups.all_from_inc[i]], n2 = n_list[groups.all_from_inc[i+1]];
                cdbl c1 = std::cos(th_all[groups.all_from_inc[i]]), c2 = std::cos(th_all[groups.all_from_inc[i+1]]);
                auto fresnel = [&](cdbl ni, cdbl nf, cdbl ci, cdbl cf) {
                    cdbl rs = (ni*ci-nf*cf)/(ni*ci+nf*cf), rp = (nf*ci-ni*cf)/(nf*ci+ni*cf);
                    double f = std::real(nf*std::conj(cf))/std::real(ni*std::conj(ci));
                    return std::make_tuple(std::norm(rs), std::norm(rp), f*std::norm(2.0*ni*ci/(ni*ci+nf*cf)), f*std::norm(2.0*ni*ci/(nf*ci+ni*cf)));
                };
                auto [Rs, Rp, Ts, Tp] = fresnel(n1, n2, c1, c2); auto [Rsb, Rpb, Tsb, Tpb] = fresnel(n2, n1, c2, c1);
                r = (pol==0?Rs:Rp); t = (pol==0?Ts:Tp); rb = (pol==0?Rsb:Rpb); tb = (pol==0?Tsb:Tpb);
            } else {
                r = (pol==0?s_fwd[s_idx].R_s:s_fwd[s_idx].R_p); t = (pol==0?s_fwd[s_idx].T_s:s_fwd[s_idx].T_p);
                rb = (pol==0?s_rev[s_idx].R_s:s_rev[s_idx].R_p); tb = (pol==0?s_rev[s_idx].T_s:s_rev[s_idx].T_p);
            }
        };

        double R, T, Rb, Tb; get_RT(0, R, T, Rb, Tb);
        double M00 = 1.0/T, M01 = -Rb/T, M10 = R/T, M11 = (T*Tb-R*Rb)/T;
        for (size_t i = 1; i < groups.num_inc_layers - 1; ++i) {
            double P = std::exp(-(4.0*PI*std::imag(n_list[groups.all_from_inc[i]]*std::cos(th_all[groups.all_from_inc[i]]))/lambda)*d_list[groups.all_from_inc[i]]);
            P = std::max(P, 1e-30); get_RT(i, R, T, Rb, Tb);
            double I00=1.0/(P*T), I01=-Rb/(P*T), I10=(P*R)/T, I11=P*(T*Tb-R*Rb)/T;
            double nM00=M00*I00+M01*I10, nM01=M00*I01+M01*I11, nM10=M10*I00+M11*I10, nM11=M10*I01+M11*I11;
            M00=nM00; M01=nM01; M10=nM10; M11=nM11;
        }
        return std::make_pair(M10/M00, 1.0/M00);
    };

    auto s = solve_inc(0); auto p = solve_inc(1);
    return { s.first, p.first, s.second, p.second, (s.first+p.first)*0.5, (s.second+p.second)*0.5 };
}

TMMResult TMMCore::calculate_incoherent(const std::vector<cdbl>& n, const std::vector<double>& d, const std::vector<char>& c, double th_0, double lambda) {
    return calculate_incoherent_ptr(n.data(), d.data(), c.data(), n.size(), th_0, lambda);
}