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
    unsigned char samples[n_samp];

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
