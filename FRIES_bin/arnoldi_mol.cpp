/*! \file
 *
 * \brief FRI applied to Arnoldi method for calculating excited-state energies
 */

#include <cstdio>
#include <iostream>
#include <ctime>
#include <FRIES/io_utils.hpp>
#include <FRIES/Ext_Libs/dcmt/dc.h>
#include <FRIES/compress_utils.hpp>
#include <FRIES/Ext_Libs/argparse.h>
#include <FRIES/Hamiltonians/heat_bathPP.hpp>
#include <FRIES/Hamiltonians/molecule.hpp>
#include <FRIES/Hamiltonians/near_uniform.hpp>
#include <FRIES/Ext_Libs/LAPACK/lapacke.h>

static const char *const usage[] = {
    "arnoldi_mol [options] [[--] args]",
    "arnoldi_mol [options]",
    NULL,
};


int main(int argc, const char * argv[]) {
    const char *hf_path = NULL;
    const char *result_dir = "./";
    const char *ini_path = NULL;
    const char *trial_path = NULL;
    unsigned int n_trial = 0;
    unsigned int target_nonz = 0;
    unsigned int matr_samp = 0;
    unsigned int max_n_dets = 0;
    unsigned int max_iter = 1000000;
    unsigned int arnoldi_interval = 10;
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('d', "hf_path", &hf_path, "Path to the directory that contains the HF output files eris.txt, hcore.txt, symm.txt, hf_en.txt, and sys_params.txt"),
        OPT_INTEGER('m', "vec_nonz", &target_nonz, "Target number of nonzero vector elements to keep after each iteration"),
        OPT_INTEGER('M', "mat_nonz", &matr_samp, "Target number of nonzero matrix elements to keep after each iteration"),
        OPT_STRING('y', "result_dir", &result_dir, "Directory in which to save output files"),
        OPT_INTEGER('p', "max_dets", &max_n_dets, "Maximum number of determinants on a single MPI process."),
        OPT_STRING('n', "ini_vecs", &ini_path, "Prefix for files containing the vectors from which to initialize the calculation (files must have names <ini_vec>dets<xx> and <ini_vec>vals<xx>, where xx is a 2-digit number ranging from 0 to (n_trial - 1), and be text files)."),
        OPT_STRING('v', "trial_vecs", &trial_path, "Prefix for files containing the vectors with which to calculate the energy (files must have names <trial_vec>dets<xx> and <trial_vec>vals<xx>, where xx is a 2-digit number ranging from 0 to (n_trial - 1), and be text files)."),
        OPT_INTEGER('k', "num_trial", &n_trial, "Number of trial vectors to use to calculate dot products with the iterates."),
        OPT_INTEGER('I', "max_iter", &max_iter, "Maximum number of iterations to run the calculation."),
        OPT_END(),
    };
    
    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\nRandomized Arnoldi method for calculating excited states", "");
    argc = argparse_parse(&argparse, argc, argv);
    
    if (hf_path == NULL) {
        fprintf(stderr, "Error: HF directory not specified.\n");
        return 0;
    }
    if (target_nonz == 0) {
        fprintf(stderr, "Error: target number of nonzero vector elements not specified\n");
        return 0;
    }
    if (matr_samp == 0) {
        fprintf(stderr, "Error: target number of nonzero matrix elements not specified\n");
        return 0;
    }
    if (max_n_dets == 0) {
        fprintf(stderr, "Error: maximum number of determinants expected on each processor not specified.\n");
        return 0;
    }
    if (!trial_path) {
        fprintf(stderr, "Error: path to trial vectors was not specified.\n");
        return 0;
    }
    if (n_trial < 2) {
        fprintf(stderr, "Error: you are either using 1 or 0 trial vectors. Consider using the power method instead of Arnoldi in this case.\n");
        return 0;
    }
    
    int n_procs = 1;
    int proc_rank = 0;
    unsigned int hf_proc;
#ifdef USE_MPI
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);
#endif
    
    // Read in data files
    hf_input in_data;
    parse_hf_input(hf_path, &in_data);
    double eps = in_data.eps;
    unsigned int n_elec = in_data.n_elec;
    unsigned int n_frz = in_data.n_frz;
    unsigned int n_orb = in_data.n_orb;
    double hf_en = in_data.hf_en;
    
    unsigned int n_elec_unf = n_elec - n_frz;
    unsigned int tot_orb = n_orb + n_frz / 2;
    
    uint8_t *symm = in_data.symm;
    Matrix<double> *h_core = in_data.hcore;
    FourDArr *eris = in_data.eris;
    
    // Rn generator
    mt_struct *rngen_ptr = get_mt_parameter_id_st(32, 521, proc_rank, (unsigned int) time(NULL));
    sgenrand_mt((uint32_t) time(NULL), rngen_ptr);
    
    // Solution vector
    unsigned int spawn_length = matr_samp * 4 / n_procs;
    size_t adder_size = spawn_length > 1000000 ? 1000000 : spawn_length;
    std::function<double(const uint8_t *)> diag_shortcut = [tot_orb, eris, h_core, n_frz, n_elec, hf_en](const uint8_t *occ_orbs) {
        return diag_matrel(occ_orbs, tot_orb, *eris, *h_core, n_frz, n_elec) - hf_en;
    };
    std::vector<DistVec<double> *> sol_vecs(n_trial);
    for (uint8_t vec_idx = 0; vec_idx < n_trial; vec_idx++) {
        sol_vecs[vec_idx] = new DistVec<double>(max_n_dets, adder_size, rngen_ptr, n_orb * 2, n_elec_unf, n_procs, diag_shortcut, NULL, 3);
    }
    size_t det_size = CEILING(2 * n_orb, 8);
    size_t det_idx;
    
    Matrix<uint8_t> symm_lookup(n_irreps, n_orb + 1);
    gen_symm_lookup(symm, symm_lookup);
    unsigned int max_n_symm = 0;
    for (det_idx = 0; det_idx < n_irreps; det_idx++) {
        if (symm_lookup[det_idx][0] > max_n_symm) {
            max_n_symm = symm_lookup[det_idx][0];
        }
    }
    
    // Initialize hash function for processors and vector
    std::vector<uint32_t> proc_scrambler(2 * n_orb);
    double loc_norm, glob_norm;
    
    if (proc_rank == 0) {
        for (det_idx = 0; det_idx < 2 * n_orb; det_idx++) {
            proc_scrambler[det_idx] = genrand_mt(rngen_ptr);
        }
        save_proc_hash(result_dir, proc_scrambler.data(), 2 * n_orb);
    }
#ifdef USE_MPI
    MPI_Bcast(proc_scrambler.data(), 2 * n_orb, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
#endif
    std::vector<uint32_t> vec_scrambler(2 * n_orb);
    for (det_idx = 0; det_idx < 2 * n_orb; det_idx++) {
        vec_scrambler[det_idx] = genrand_mt(rngen_ptr);
    }
    
    uint8_t hf_det[det_size];
    gen_hf_bitstring(n_orb, n_elec - n_frz, hf_det);
    hf_proc = sol_vecs[0]->idx_to_proc(hf_det);
    
    uint8_t tmp_orbs[n_elec_unf];
    uint8_t (*orb_indices1)[4] = (uint8_t (*)[4])malloc(sizeof(char) * 4 * spawn_length);
    
# pragma mark Set up trial vectors
    std::vector<DistVec<double>> trial_vecs;
    trial_vecs.reserve(n_trial);
    std::vector<DistVec<double>> htrial_vecs;
    htrial_vecs.reserve(n_trial);
    size_t n_ex = n_orb * n_orb * n_elec_unf * n_elec_unf;
    
    unsigned int n_trial_dets = 0;
    
    char vec_path[300];
    for (unsigned int trial_idx = 0; trial_idx < n_trial; trial_idx++) {
        if (proc_rank == 0) {
            sprintf(vec_path, "%s%02d", trial_path, trial_idx);
            n_trial_dets = (unsigned int) load_vec_txt(vec_path, load_dets, load_vals, DOUB);
        }
#ifdef USE_MPI
        MPI_Bcast(&n_trial_dets, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
#endif
//        trial_vecs[trial_idx] = new DistVec<double>(n_trial_dets, n_trial_dets, rngen_ptr, n_orb * 2, n_elec_unf, n_procs, proc_scrambler);
//        htrial_vecs[trial_idx] = new DistVec<double>(n_trial_dets * n_ex / n_procs, n_trial_dets * n_ex / n_procs, rngen_ptr, n_orb * 2, n_elec_unf, n_procs, diag_shortcut, (double *)NULL, 2, proc_scrambler);
        trial_vecs.emplace_back(n_trial_dets, n_trial_dets, n_orb * 2, n_elec_unf, n_procs, proc_scrambler, vec_scrambler);
        htrial_vecs.emplace_back(n_trial_dets * n_ex / n_procs, n_trial_dets * n_ex / n_procs, n_orb * 2, n_elec_unf, n_procs, diag_shortcut, (double *)NULL, 2, proc_scrambler, vec_scrambler);
        
        for (det_idx = 0; det_idx < n_trial_dets; det_idx++) {
            trial_vecs[trial_idx].add(load_dets[det_idx], load_vals[det_idx], 1);
            htrial_vecs[trial_idx].add(load_dets[det_idx], load_vals[det_idx], 1);
        }
    }
//    uintmax_t **trial_hashes = (uintmax_t **)malloc(sizeof(uintmax_t *) * n_trial);
//    uintmax_t **htrial_hashes = (uintmax_t **)malloc(sizeof(uintmax_t *) * n_trial);
    Matrix<uintmax_t *>trial_hashes(n_trial, n_trial);
    Matrix<uintmax_t *>htrial_hashes(n_trial, n_trial);
    for (uint8_t trial_idx = 0; trial_idx < n_trial; trial_idx++) {
        DistVec<double> *curr_trial = trial_vecs[trial_idx];
        curr_trial->perform_add();
        curr_trial->collect_procs();
        for (uint8_t vec_idx = 0; vec_idx < n_trial; vec_idx++) {
            trial_hashes(trial_idx, vec_idx) = (uintmax_t *)malloc(sizeof(uintmax_t) * curr_trial->curr_size());
            for (det_idx = 0; det_idx < curr_trial->curr_size(); det_idx++) {
                trial_hashes(trial_idx, vec_idx)[det_idx] = sol_vecs[vec_idx]->idx_to_hash(curr_trial->indices()[det_idx], tmp_orbs);
            }
        }
        
        DistVec<double> &curr_htrial = htrial_vecs[trial_idx];
        curr_htrial.perform_add();
        h_op_offdiag(curr_htrial, symm, tot_orb, *eris, *h_core, (uint8_t *)orb_indices1, n_frz, n_elec_unf, 1, 1);
        curr_htrial.set_curr_vec_idx(0);
        h_op_diag(curr_htrial, 0, 0, 1);
        curr_htrial.add_vecs(0, 1);
        curr_htrial.collect_procs();
        htrial_hashes[trial_idx] = (uintmax_t *)malloc(sizeof(uintmax_t) * curr_htrial.curr_size());
        for (det_idx = 0; det_idx < curr_htrial.curr_size(); det_idx++) {
            htrial_hashes[trial_idx][det_idx] = sol_vec.idx_to_hash(curr_htrial.indices()[det_idx], tmp_orbs);
        }
    }
    
    // Count # single/double excitations from HF
//    sol_vec.gen_orb_list(hf_det, tmp_orbs);
//    size_t n_hf_doub = doub_ex_symm(hf_det, tmp_orbs, n_elec_unf, n_orb, orb_indices1, symm);
//    size_t n_hf_sing = count_singex(hf_det, tmp_orbs, symm, n_orb, symm_lookup, n_elec_unf);
//    double p_doub = (double) n_hf_doub / (n_hf_sing + n_hf_doub);
    
    char file_path[300];
    FILE *bmat_file = NULL;
    FILE *dmat_file = NULL;
    
#pragma mark Initialize solution vector
    if (!ini_path) {
        ini_path = trial_path;
    }
    Matrix<uint8_t> *load_dets = new Matrix<uint8_t>(max_n_dets, det_size);
    for (unsigned int vec_idx = 0; vec_idx < n_trial; vec_idx++) {
        sol_vecs[vec_idx]->set_curr_vec_idx(2);
        double *load_vals = (double *)sol_vecs[vec_idx]->values();
        
        size_t n_dets = load_vec_txt(ini_path, *load_dets, load_vals, DOUB);
        
        for (det_idx = 0; det_idx < n_dets; det_idx++) {
            sol_vecs[vec_idx]->add((*load_dets)[det_idx], load_vals[det_idx], 1);
        }
        n_dets++; // just to be safe
        bzero(load_vals, n_dets * sizeof(double));
        
        sol_vecs[vec_idx]->perform_add();
        sol_vecs[vec_idx]->set_curr_vec_idx(0);
    }
    delete load_dets;
    
    if (proc_rank == hf_proc) {
        // Setup output files
        strcpy(file_path, result_dir);
        strcat(file_path, "b_matrix.txt");
        bmat_file = fopen(file_path, "a");
        if (!bmat_file) {
            fprintf(stderr, "Could not open file for writing in directory %s\n", result_dir);
        }
        
        strcpy(file_path, result_dir);
        strcat(file_path, "d_matrix.txt");
        dmat_file = fopen(file_path, "a");
        
        strcpy(file_path, result_dir);
        strcat(file_path, "params.txt");
        FILE *param_f = fopen(file_path, "w");
        fprintf(param_f, "Arnoldi calculation\nHF path: %s\nepsilon (imaginary time step): %lf\nMatrix nonzero: %u\nVector nonzero: %u\n", hf_path, eps, matr_samp, target_nonz);
        if (ini_path) {
            fprintf(param_f, "Initializing calculation from vector files with prefix %s\n", ini_path);
        }
        else {
            fprintf(param_f, "Initializing calculation from HF unit vector\n");
        }
        fclose(param_f);
    }
    
//    size_t n_states = n_elec_unf > (n_orb - n_elec_unf / 2) ? n_elec_unf : n_orb - n_elec_unf / 2;
//    Matrix<double> subwt_mem(spawn_length, n_states);
//    uint16_t *sub_sizes = (uint16_t *)malloc(sizeof(uint16_t) * spawn_length);
//    unsigned int *ndiv_vec = (unsigned int *)malloc(sizeof(unsigned int) * spawn_length);
//    double *comp_vec1 = (double *)malloc(sizeof(double) * spawn_length);
//    double *comp_vec2 = (double *)malloc(sizeof(double) * spawn_length);
//    size_t (*comp_idx)[2] = (size_t (*)[2])malloc(sizeof(size_t) * 2 * spawn_length);
//    size_t comp_len;
//    size_t *det_indices1 = (size_t *)malloc(sizeof(size_t) * 2 * spawn_length);
//    size_t *det_indices2 = &det_indices1[spawn_length];
//    uint8_t (*orb_indices2)[4] = (uint8_t (*)[4])malloc(sizeof(uint8_t) * 4 * spawn_length);
//    unsigned int unocc_symm_cts[n_irreps][2];
//    Matrix<bool> keep_idx(spawn_length, n_states);
//    double *wt_remain = (double *)calloc(spawn_length, sizeof(double));
//    size_t samp_idx, weight_idx;
    
//    hb_info *hb_probs = set_up(tot_orb, n_orb, *eris);
    
    // Parameters for systematic sampling
    double rn_sys = 0;
    double loc_norms[n_procs];
    for (unsigned int vec_idx = 0; vec_idx < n_trial; vec_idx++) {
        if (sol_vecs[vec_idx]->max_size() > max_n_dets) {
            max_n_dets = (unsigned int)sol_vecs[vec_idx]->max_size();
        }
    }
    size_t *srt_arr = (size_t *)malloc(sizeof(size_t) * max_n_dets);
    for (det_idx = 0; det_idx < max_n_dets; det_idx++) {
        srt_arr[det_idx] = det_idx;
    }
    std::vector<bool> keep_exact(max_n_dets, false);
    
    for (uint32_t iteration = 0; iteration < max_iter; iteration++) {
        // Initialize the solution vectors
        for (uint16_t vec_idx = 0; vec_idx < n_trial; vec_idx++) {
            sol_vecs[vec_idx]->copy_vec(2, 0);
        }
        
        for (uint16_t krylov_idx = 0; krylov_idx < n_krylov; krylov_idx++) {
#pragma mark Krylov dot products
            for (uint16_t trial_idx = 0; trial_idx < n_trial; trial_idx++) {
                DistVec<double> *curr_trial = trial_vecs[trial_idx];
                DistVec<double> *curr_htrial = htrial_vecs[trial_idx];
                for (uint16_t vec_idx = 0; vec_idx < n_trial; vec_idx++) {
                    double d_prod = sol_vecs[vec_idx]->dot(curr_trial->indices(), curr_trial->values(), curr_trial->curr_size(), trial_hashes(trial_idx, vec_idx));
                    d_prod = sum_mpi(d_prod, proc_rank, n_procs);
                    if (proc_rank == hf_proc) {
                        fprintf(dmat_file, "%lf,", d_prod);
                    }
                    
                    d_prod = sol_vecs[vec_idx]->dot(curr_htrial->indices(), curr_htrial->values(), curr_htrial->curr_size(), htrial_hashes(trial_idx, vec_idx));
                    d_prod = sum_mpi(d_prod, proc_rank, n_procs);
                    if (proc_rank == hf_proc) {
                        fprintf(bmat_file, "%lf,", d_prod);
                    }
                }
            }
            if (proc_rank == hf_proc) {
                fprintf(dmat_file, "\n");
                fprintf(bmat_file, "\n");
                fflush(dmat_file);
                fflush(bmat_file);
            }
            
# pragma mark Power iteration
            size_t new_max_dets = max_n_dets;
            for (uint16_t vec_idx = 0; vec_idx < n_trial; vec_idx++) {
                DistVec<double> *curr_vec = sol_vecs[vec_idx];
                h_op_offdiag(*curr_vec, symm, tot_orb, *eris, *h_core, (uint8_t *)orb_indices1, n_frz, n_elec_unf, 1, -eps);
                curr_vec->set_curr_vec_idx(0);
                h_op_diag(*curr_vec, 0, 1, -eps);
                curr_vec->add_vecs(0, 1);
                if (curr_vec->max_size() > new_max_dets) {
                    new_max_dets = curr_vec->max_size();
                }
            }
            
            if (new_max_dets > max_n_dets) {
                keep_exact.resize(new_max_dets, false);
                srt_arr = (size_t *)realloc(srt_arr, sizeof(size_t) * new_max_dets);
                for (; max_n_dets < new_max_dets; max_n_dets++) {
                    srt_arr[max_n_dets] = max_n_dets;
                }
            }
#pragma mark Vector compression
            for (uint16_t vec_idx = 0; vec_idx < n_trial; vec_idx++) {
                unsigned int n_samp = target_nonz;
                double glob_norm;
                loc_norms[proc_rank] = find_preserve(sol_vecs[vec_idx]->values(), srt_arr, keep_exact, sol_vecs[vec_idx]->curr_size(), &n_samp, &glob_norm);
                if (proc_rank == 0) {
                    rn_sys = genrand_mt(rngen_ptr) / (1. + UINT32_MAX);
                }
#ifdef USE_MPI
                MPI_Allgather(MPI_IN_PLACE, 0, MPI_DOUBLE, loc_norms, 1, MPI_DOUBLE, MPI_COMM_WORLD);
#endif
                sys_comp(sol_vecs[vec_idx]->values(), sol_vecs[vec_idx]->curr_size(), loc_norms, n_samp, keep_exact, rn_sys);
                for (det_idx = 0; det_idx < sol_vecs[vec_idx]->curr_size(); det_idx++) {
                    if (keep_exact[det_idx]) {
                        sol_vecs[vec_idx]->del_at_pos(det_idx);
                        keep_exact[det_idx] = 0;
                    }
                }
            }
        }
        //        LAPACKE_dgeev(LAPACK_ROW_MAJOR, 'N', 'N', n_trial, krylov_mat.data(), n_trial, energies_r, energies_i, NULL, n_trial, NULL, n_trial);
    }
    
    if (proc_rank == hf_proc) {
        fclose(bmat_file);
        fclose(dmat_file);
    }
#ifdef USE_MPI
    MPI_Finalize();
#endif
    return 0;
}
