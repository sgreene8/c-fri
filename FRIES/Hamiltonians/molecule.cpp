/*! \file
 * \brief Utilities for a Hamiltonian describing a molecular system
 */

#include "molecule.hpp"


double doub_matr_el_nosgn(uint8_t *chosen_orbs, unsigned int n_orbs,
                          const FourDArr &eris, unsigned int n_frozen) {
    uint8_t sp0, sp1, sp2, sp3;
    unsigned int adj_n_orb = n_orbs - n_frozen / 2;
    sp0 = chosen_orbs[0];
    sp1 = chosen_orbs[1];
    int same_sp = sp0 / adj_n_orb == sp1 / adj_n_orb;
    sp0 = (sp0 % adj_n_orb) + n_frozen / 2;
    sp1 = (sp1 % adj_n_orb) + n_frozen / 2;
    sp2 = (chosen_orbs[2] % adj_n_orb) + n_frozen / 2;
    sp3 = (chosen_orbs[3] % adj_n_orb) + n_frozen / 2;
    
    double mat_el = eris(sp0, sp1, sp2, sp3);
    if (same_sp)
    mat_el -= eris(sp0, sp1, sp3, sp2);
    return mat_el;
}


double sing_matr_el_nosgn(uint8_t *chosen_orbs, uint8_t *occ_orbs,
                          unsigned int n_orbs, const FourDArr &eris,
                          const Matrix<double> &h_core, unsigned int n_frozen,
                          unsigned int n_elec) {
    unsigned int half_frz = n_frozen / 2;
    uint8_t occ_spa = (chosen_orbs[0] % (n_orbs - half_frz)) + half_frz;
    uint8_t unocc_spa = (chosen_orbs[1] % (n_orbs - half_frz)) + half_frz;
    unsigned int occ_spin = chosen_orbs[0] / (n_orbs - half_frz);
    double mat_el = h_core(occ_spa, unocc_spa);
    unsigned int j;
    
    for (j = 0; j < half_frz; j++) {
        mat_el += eris(occ_spa, j, unocc_spa, j) * 2;
        // single-count exchange term
        mat_el -= eris(occ_spa, j, j, unocc_spa);
    }
    for (j = 0; j < n_elec / 2; j++) {
        mat_el += eris(occ_spa, occ_orbs[j] + half_frz, unocc_spa, occ_orbs[j] + half_frz);
        if (occ_spin == 0) {
            mat_el -= eris(occ_spa, occ_orbs[j] + half_frz, occ_orbs[j] + half_frz, unocc_spa);
        }
    }
    for (j = n_elec / 2; j < n_elec; j++) {
        mat_el += eris(occ_spa, occ_orbs[j] - n_orbs + half_frz * 2, unocc_spa, occ_orbs[j] - n_orbs + half_frz * 2);
        if (occ_spin == 1) {
            mat_el -= eris(occ_spa, occ_orbs[j] - n_orbs + half_frz * 2, occ_orbs[j] - n_orbs + half_frz * 2, unocc_spa);
        }
    }
    return mat_el;
}


size_t doub_ex_symm(uint8_t *det, uint8_t *occ_orbs, unsigned int num_elec,
                    unsigned int num_orb, uint8_t res_arr[][4], uint8_t *symm) {
    uint8_t i, i_orb, j, j_orb, k, l;
    unsigned int idx = 0;
    // Different-spin excitations
    for (i = 0; i < num_elec / 2; i++) {
        i_orb = occ_orbs[i];
        for (j = num_elec / 2; j < num_elec; j++) {
            j_orb = occ_orbs[j];
            for (k = 0; k < num_orb; k++) {
                if (!(read_bit(det, k))) {
                    for (l = num_orb; l < 2 * num_orb; l++) {
                        if (!(read_bit(det, l)) && (symm[i_orb] ^ symm[j_orb - num_orb] ^ symm[k] ^ symm[l - num_orb]) == 0) {
                            res_arr[idx][0] = i_orb;
                            res_arr[idx][1] = j_orb;
                            res_arr[idx][2] = k;
                            res_arr[idx][3] = l;
                            idx++;
                        }
                    }
                }
            }
        }
    }
    // Same-spin (up) excitations
    for (i = 0; i < num_elec / 2; i++) {
        i_orb = occ_orbs[i];
        for (j = i + 1; j < num_elec / 2; j++) {
            j_orb = occ_orbs[j];
            for (k = 0; k < num_orb; k++) {
                if (!(read_bit(det, k))) {
                    for (l = k + 1; l < num_orb; l++) {
                        if (!(read_bit(det, l)) && (symm[i_orb] ^ symm[j_orb] ^ symm[k] ^ symm[l]) == 0) {
                            res_arr[idx][0] = i_orb;
                            res_arr[idx][1] = j_orb;
                            res_arr[idx][2] = k;
                            res_arr[idx][3] = l;
                            idx++;
                        }
                    }
                }
            }
        }
    }
    // Same-spin (down) excitations
    for (i = num_elec / 2; i < num_elec; i++) {
        i_orb = occ_orbs[i];
        for (j = i + 1; j < num_elec; j++) {
            j_orb = occ_orbs[j];
            for (k = num_orb; k < 2 * num_orb; k++) {
                if (!(read_bit(det, k))) {
                    for (l = k + 1; l < 2 * num_orb; l++) {
                        if (!(read_bit(det, l)) && (symm[i_orb - num_orb] ^
                                                    symm[j_orb - num_orb] ^
                                                    symm[k - num_orb] ^ symm[l - num_orb]) == 0) {
                            res_arr[idx][0] = i_orb;
                            res_arr[idx][1] = j_orb;
                            res_arr[idx][2] = k;
                            res_arr[idx][3] = l;
                            idx++;
                        }
                    }
                }
            }
        }
    }
    return idx;
}


size_t sing_ex_symm(uint8_t *det, uint8_t *occ_orbs, unsigned int num_elec,
                    unsigned int num_orb, uint8_t res_arr[][2], uint8_t *symm) {
    uint8_t i, i_orb, a;
    unsigned int idx = 0;
    for (i = 0; i < num_elec / 2; i++) { // spin-up excitations
        i_orb = occ_orbs[i];
        for (a = 0; a < num_orb; a++) {
            if (!(read_bit(det, a)) && (symm[i_orb] == symm[a])) {
                res_arr[idx][0] = i_orb;
                res_arr[idx][1] = a;
                idx++;
            }
        }
    }
    for (i = num_elec / 2; i < num_elec; i++) { // spin-down excitations
        i_orb = occ_orbs[i];
        for (a = num_orb; a < 2 * num_orb; a++) {
            if (!(read_bit(det, a)) && symm[i_orb - num_orb] == symm[a - num_orb]) {
                res_arr[idx][0] = i_orb;
                res_arr[idx][1] = a;
                idx++;
            }
        }
    }
    return idx;
}


void h_op(DistVec<double> &vec, uint8_t *symm, unsigned int n_orbs,
          const FourDArr &eris, const Matrix<double> &h_core,
          uint8_t *orbs_scratch, unsigned int n_frozen,
          unsigned int n_elec, double id_fac, double h_fac, double hf_en) {
    size_t det_idx, ex_idx;
    unsigned int unf_orbs = n_orbs - n_frozen / 2;
    uint8_t n_bytes = CEILING(vec.n_bits(), 8);
    uint8_t new_det[n_bytes];
    for (det_idx = 0; det_idx < vec.curr_size(); det_idx++) {
        double *curr_el = vec[det_idx];
        uint8_t *curr_det = vec.indices()[det_idx];
        if (*curr_el == 0) {
            continue;
        }
        
        uint8_t *occ_orbs = vec.orbs_at_pos(det_idx);
        uint8_t (*sing_ex_orbs)[2] = (uint8_t (*)[2])orbs_scratch;
        size_t n_sing = sing_ex_symm(curr_det, occ_orbs, n_elec, unf_orbs, sing_ex_orbs, symm);
        for (ex_idx = 0; ex_idx < n_sing; ex_idx++) {
            double matr_el = sing_matr_el_nosgn(sing_ex_orbs[ex_idx], occ_orbs, n_orbs, eris, h_core, n_frozen, n_elec);
            memcpy(new_det, curr_det, n_bytes);
            matr_el *= sing_det_parity(new_det, sing_ex_orbs[ex_idx]);
            matr_el *= *curr_el * h_fac;
            vec.add(new_det, matr_el, 1);
        }
        
        uint8_t (*doub_ex_orbs)[4] = (uint8_t (*)[4])orbs_scratch;
        size_t n_doub = doub_ex_symm(curr_det, occ_orbs, n_elec, unf_orbs, doub_ex_orbs, symm);
        for (ex_idx = 0; ex_idx < n_doub; ex_idx++) {
            double matr_el = doub_matr_el_nosgn(doub_ex_orbs[ex_idx], n_orbs, eris, n_frozen);
            memcpy(new_det, curr_det, n_bytes);
            matr_el *= doub_det_parity(new_det, doub_ex_orbs[ex_idx]);
            matr_el *= *curr_el * h_fac;
            vec.add(new_det, matr_el, 1);
        }

        double *diag_el = vec.matr_el_at_pos(det_idx);
        if (isnan(*diag_el)) {
            *diag_el = diag_matrel(occ_orbs, n_orbs, eris, h_core, n_frozen, n_elec + n_frozen) - hf_en;
        }
        *curr_el *= (id_fac + h_fac * (*diag_el));
    }
    vec.perform_add();
}


size_t count_doub_nosymm(unsigned int num_elec, unsigned int num_orb) {
    unsigned int num_unocc = num_orb - num_elec / 2;
    return num_elec * (num_elec / 2 - 1) * num_unocc * (num_unocc - 1) / 2 +
    num_elec / 2 * num_elec / 2 * num_unocc * num_unocc;
}


size_t gen_hf_ex(uint8_t *hf_det, uint8_t *hf_occ, unsigned int num_elec,
                 unsigned int n_orb, uint8_t *orb_symm, const FourDArr &eris,
                 unsigned int n_frozen, Matrix<uint8_t> &ex_dets, double *ex_mel) {
    unsigned int num_unf_orb = n_orb - n_frozen / 2;
    size_t max_n_doub = count_doub_nosymm(num_elec, num_unf_orb);
    uint8_t ex_arr[max_n_doub][4];
    size_t num_hf_doub = doub_ex_symm(hf_det, hf_occ, num_elec, num_unf_orb, ex_arr, orb_symm);
    size_t idx;
    size_t n_bytes = ex_dets.cols();
    double matr_el;
    for (idx = 0; idx < num_hf_doub; idx++) {
        memcpy(ex_dets[idx], hf_det, n_bytes);
        matr_el = doub_matr_el_nosgn(&ex_arr[idx][0], n_orb, eris, n_frozen);
        matr_el *= doub_det_parity(ex_dets[idx], &ex_arr[idx][0]);
        ex_mel[idx] = matr_el;
    }
    return num_hf_doub;
}

size_t count_singex(uint8_t *det, const uint8_t *occ_orbs, const uint8_t *orb_symm,
                    unsigned int num_orb, const Matrix<uint8_t> &lookup_tabl,
                    unsigned int num_elec) {
    size_t num_ex = 0;
    unsigned int elec_idx, symm_idx;
    uint8_t elec_symm, elec_orb;
    int elec_spin;
    for (elec_idx = 0; elec_idx < num_elec; elec_idx++) {
        elec_orb = occ_orbs[elec_idx];
        elec_symm = orb_symm[elec_orb % num_orb];
        elec_spin = elec_orb / num_orb;
        for (symm_idx = 0; symm_idx < lookup_tabl[elec_symm][0]; symm_idx++) {
            if (!(read_bit(det, lookup_tabl[elec_symm][symm_idx + 1] +
                           num_orb * elec_spin))) {
                num_ex += 1;
            }
        }
    }
    return num_ex;
}

double diag_matrel(const uint8_t *occ_orbs, unsigned int n_orbs,
                   const FourDArr &eris, const Matrix<double> &h_core,
                   unsigned int n_frozen, unsigned int n_elec) {
    unsigned int j, k, elec_1, elec_2;
    double matr_sum = 0;
    unsigned int n_e_unf = n_elec - n_frozen;
    
    for (j = 0; j < n_frozen / 2; j++) {
        matr_sum += h_core(j, j) * 2;
        matr_sum += eris(j, j, j, j);
        for (k = j + 1; k < n_frozen / 2; k++) {
            matr_sum += eris(j, k, j, k) * 4;
            matr_sum -= eris(j, k, k, j) * 2;
        }
    }
    for (j = 0; j < n_e_unf / 2; j ++) {
        elec_1 = occ_orbs[j] + n_frozen / 2;
        matr_sum += h_core(elec_1, elec_1);
        for (k = 0; k < n_frozen / 2; k++) {
            matr_sum += eris(elec_1, k, elec_1, k) * 2;
            matr_sum -= eris(elec_1, k, k, elec_1);
        }
        for (k = j + 1; k < n_e_unf / 2; k++) {
            elec_2 = occ_orbs[k] + n_frozen / 2;
            matr_sum += eris(elec_1, elec_2, elec_1, elec_2);
            matr_sum -= eris(elec_1, elec_2, elec_2, elec_1);
        }
        for (k = n_e_unf / 2; k <  n_e_unf; k++) {
            elec_2 = occ_orbs[k] + n_frozen - n_orbs;
            matr_sum += eris(elec_1, elec_2, elec_1, elec_2);
        }
    }
    for (j = n_e_unf / 2; j < n_e_unf; j++) {
        elec_1 = occ_orbs[j] + n_frozen - n_orbs;
        matr_sum += h_core(elec_1, elec_1);
        for (k = 0; k < n_frozen / 2; k++) {
            matr_sum += eris(elec_1, k, elec_1, k) * 2;
            matr_sum -= eris(elec_1, k, k, elec_1);
        }
        for (k = j + 1; k < n_e_unf; k++) {
            elec_2 = occ_orbs[k] + n_frozen - n_orbs;
            matr_sum += eris(elec_1, elec_2, elec_1, elec_2);
            matr_sum -= eris(elec_1, elec_2, elec_2, elec_1);
        }
    }
    return matr_sum;
}


uint8_t find_nth_virt_symm(uint8_t *det, uint8_t spin_orbs, uint8_t irrep, uint8_t n,
                           const Matrix<uint8_t> &lookup_tabl) {
    unsigned int num_u2 = lookup_tabl(irrep, 0);
    uint8_t u2_orb;
    uint8_t virt_idx = 0;
    for (unsigned int orb_idx = 0; orb_idx < num_u2; orb_idx++) {
        u2_orb = lookup_tabl(irrep, orb_idx + 1) + spin_orbs;
        if (read_bit(det, u2_orb) == 0) {
            if (virt_idx == n) {
                return u2_orb;
            }
            virt_idx++;
        }
    }
    return 255;
}


void gen_symm_lookup(uint8_t *orb_symm,
                     Matrix<uint8_t> &lookup_tabl) {
    uint8_t symm;
    size_t n_symm = lookup_tabl.rows();
    size_t n_orb = lookup_tabl.cols() - 1;
    for (unsigned int idx = 0; idx < n_symm; idx++) {
        lookup_tabl(idx, 0) = 0;
    }
    for (unsigned int idx = 0; idx < n_orb; idx++) {
        symm = orb_symm[idx];
        unsigned int count = lookup_tabl(symm, 0);
        lookup_tabl(symm, 1 + count) = idx;
        count++;
        lookup_tabl(symm, 0) = count;
    }
}

void print_symm_lookup(Matrix<uint8_t> &lookup_tabl) {
    size_t n_symm = lookup_tabl.rows();
    for (unsigned int idx = 0; idx < n_symm; idx++) {
        printf("%u: ", idx);
        for (unsigned int orb_idx = 0; orb_idx < lookup_tabl(idx, 0); orb_idx++) {
            printf("%u, ", lookup_tabl(idx, 1 + orb_idx));
        }
        printf("\n");
    }
}
