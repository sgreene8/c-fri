/*! \file
 *
 * \brief Tests for stochastic compression functions
 */

#include "catch.hpp"
#include "inputs.hpp"
#include <stdio.h>
#include <FRIES/compress_utils.hpp>

TEST_CASE("Test alias method", "[alias]") {
    using namespace test_inputs;
    double probs[] = {0.10125, 0.05625, 0.0875 , 0.03   , 0.095  , 0.05375, 0.095  ,
        0.0875 , 0.0625 , 0.33125};
    unsigned int n_states = 10;
    unsigned int n_samp = 10;
    unsigned int n_iter = 10000;

    double alias_probs[n_states];
    unsigned int aliases[n_states];
    setup_alias(probs, aliases, alias_probs, n_states);

    mt_struct *rngen_ptr = get_mt_parameter_id_st(32, 521, 0, 0);
    sgenrand_mt(0, rngen_ptr);
    uint8_t samples[n_samp];

    unsigned int cumu_samp[n_states];
    unsigned int iter_idx, samp_idx;
    char buf[200];
    sprintf(buf, "%s/alias.txt", out_path.c_str());
    FILE *cumu_f = fopen(buf, "w");
    REQUIRE(cumu_f != NULL);
    for (samp_idx = 0; samp_idx < n_states; samp_idx++) {
        cumu_samp[samp_idx] = 0;
    }

    for (iter_idx = 0; iter_idx < n_iter; iter_idx++) {
        sample_alias(aliases, alias_probs, n_states, samples, n_samp, 1, rngen_ptr);
        for (samp_idx = 0; samp_idx < n_samp; samp_idx++) {
            cumu_samp[samples[samp_idx]]++;
        }
        for (samp_idx = 0; samp_idx < n_states; samp_idx++) {
            fprintf(cumu_f, "%lf,", cumu_samp[samp_idx] / (iter_idx + 1.) / n_samp - probs[samp_idx]);
        }
        fprintf(cumu_f, "\n");
    }

    double max_diff = 0;
    double difference;
    for (samp_idx = 0; samp_idx < n_states; samp_idx++) {
        difference = fabs(cumu_samp[samp_idx] / 1. / n_samp / n_iter - probs[samp_idx]);
        if (difference > max_diff) {
            max_diff = difference;
        }
    }
    REQUIRE(max_diff == Approx(0).margin(1e-3));

    fclose(cumu_f);
}


TEST_CASE("Test calculation of observables from systematic sampling", "[sys_obs]") {
    mt_struct *rngen_ptr = get_mt_parameter_id_st(32, 521, 0, (unsigned int) time(NULL));
    sgenrand_mt((uint32_t) time(NULL), rngen_ptr);
    
    unsigned int input_len = 10;
    size_t num_rns = 10;
    double input_vec[input_len];
    double tmp_vec[input_len];
    double observables[num_rns];
    size_t vec_srt[input_len];
    for (size_t el_idx = 0; el_idx < input_len; el_idx++) {
        vec_srt[el_idx] = el_idx;
    }
    std::vector<bool> vec_keep1(input_len, false);
    std::vector<bool> vec_keep2(input_len, false);
    std::function<double(size_t)> obs_fxn = [](size_t idx) {
        return idx + 1;
    };
    
    for (size_t test_idx = 0; test_idx < 100; test_idx++) {
        unsigned int n_samp = (test_idx % (input_len / 2)) + 1;
        double tot_norm;
        for (size_t el_idx = 0; el_idx < input_len; el_idx++) {
            input_vec[el_idx] = genrand_mt(rngen_ptr) / (1. + UINT32_MAX);
            vec_keep1[el_idx] = false;
        }
        double samp_norm = find_preserve(input_vec, vec_srt, vec_keep1, input_len, &n_samp, &tot_norm);
        sys_obs(input_vec, input_len, &samp_norm, n_samp, vec_keep1, obs_fxn, observables, num_rns);
        
        for (size_t rn_idx = 0; rn_idx < num_rns; rn_idx++) {
            for (size_t el_idx = 0; el_idx < input_len; el_idx++) {
                tmp_vec[el_idx] = input_vec[el_idx];
                vec_keep2[el_idx] = vec_keep1[el_idx];
            }
            double tmp_norm = samp_norm;
            double rn = rn_idx / (double)num_rns;
            sys_comp(tmp_vec, input_len, &tmp_norm, n_samp, vec_keep2, rn);
            
            double comp_obs = 0;
            for (size_t el_idx = 0; el_idx < input_len; el_idx++) {
                comp_obs += obs_fxn(el_idx) * tmp_vec[el_idx] * tmp_vec[el_idx];
            }
            if (fabs(comp_obs - observables[rn_idx]) > 1e-7) {
                printf("Observable-based systematic compression failed for rn = %lf, n_samp = %lu\nVector:\n", rn, (test_idx % (input_len / 2)) + 1);
                for (size_t el_idx = 0; el_idx < input_len; el_idx++) {
                    printf("%lf\n", input_vec[el_idx]);
                }
                REQUIRE(comp_obs == Approx(observables[rn_idx]).margin(1e-7));
            }
        }
    }
}
