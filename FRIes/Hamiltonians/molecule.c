/*! \file Utilities for a Hamiltonian describing a molecular system */

#include "molecule.h"


double doub_matr_el_nosgn(unsigned char *chosen_orbs, unsigned int n_orbs,
                          double (* eris)[n_orbs][n_orbs][n_orbs], unsigned int n_frozen) {
    unsigned char sp0, sp1, sp2, sp3;
    unsigned int adj_n_orb = n_orbs - n_frozen / 2;
    sp0 = chosen_orbs[0];
    sp1 = chosen_orbs[1];
    int same_sp = sp0 / adj_n_orb == sp1 / adj_n_orb;
    sp0 = (sp0 % adj_n_orb) + n_frozen / 2;
    sp1 = (sp1 % adj_n_orb) + n_frozen / 2;
    sp2 = (chosen_orbs[2] % adj_n_orb) + n_frozen / 2;
    sp3 = (chosen_orbs[3] % adj_n_orb) + n_frozen / 2;
    
    double mat_el = eris[sp0][sp1][sp2][sp3];
    if (same_sp)
    mat_el -= eris[sp0][sp1][sp3][sp2];
    return mat_el;
}


double sing_matr_el_nosgn(unsigned char *chosen_orbs, unsigned char *occ_orbs,
                          unsigned int n_orbs, double (* eris)[n_orbs][n_orbs][n_orbs],
                          double (* h_core)[n_orbs], unsigned int n_frozen,
                          unsigned int n_elec) {
    unsigned int half_frz = n_frozen / 2;
    unsigned char occ_spa = (chosen_orbs[0] % (n_orbs - half_frz)) + half_frz;
    unsigned char unocc_spa = (chosen_orbs[1] % (n_orbs - half_frz)) + half_frz;
    unsigned int occ_spin = chosen_orbs[0] / (n_orbs - half_frz);
    double mat_el = h_core[occ_spa][unocc_spa];
    unsigned int j;
    
    for (j = 0; j < half_frz; j++) {
        mat_el += eris[occ_spa][j][unocc_spa][j] * 2;
        // single-count exchange term
        mat_el -= eris[occ_spa][j][j][unocc_spa];
    }
    for (j = 0; j < n_elec / 2; j++) {
        mat_el += eris[occ_spa][occ_orbs[j] + half_frz][unocc_spa][occ_orbs[j] + half_frz];
        if (occ_spin == 0) {
            mat_el -= eris[occ_spa][occ_orbs[j] + half_frz][occ_orbs[j] + half_frz][unocc_spa];
        }
    }
    for (j = n_elec / 2; j < n_elec; j++) {
        mat_el += eris[occ_spa][occ_orbs[j] - n_orbs + half_frz * 2][unocc_spa][occ_orbs[j] - n_orbs + half_frz * 2];
        if (occ_spin == 1) {
            mat_el -= eris[occ_spa][occ_orbs[j] - n_orbs + half_frz * 2][occ_orbs[j] - n_orbs + half_frz * 2][unocc_spa];
        }
    }
    return mat_el;
}


size_t doub_ex_symm(long long det, unsigned char *occ_orbs, unsigned int num_elec,
                    unsigned int num_orb, unsigned char res_arr[][4], unsigned char *symm) {
    unsigned char i, i_orb, j, j_orb, k, l;
    unsigned int idx = 0;
    // Different-spin excitations
    for (i = 0; i < num_elec / 2; i++) {
        i_orb = occ_orbs[i];
        for (j = num_elec / 2; j < num_elec; j++) {
            j_orb = occ_orbs[j];
            for (k = 0; k < num_orb; k++) {
                if (!(det & (1LL << k))) {
                    for (l = num_orb; l < 2 * num_orb; l++) {
                        if (!(det & (1LL << l)) && (symm[i_orb] ^ symm[j_orb - num_orb] ^ symm[k] ^ symm[l - num_orb]) == 0) {
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
                if (!(det & (1LL << k))) {
                    for (l = k + 1; l < num_orb; l++) {
                        if (!(det & (1LL << l)) && (symm[i_orb] ^ symm[j_orb] ^ symm[k] ^ symm[l]) == 0) {
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
                if (!(det & (1LL << k))) {
                    for (l = k + 1; l < 2 * num_orb; l++) {
                        if (!(det & (1LL << l)) && (symm[i_orb - num_orb] ^
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


size_t count_doub_nosymm(unsigned int num_elec, unsigned int num_orb) {
    unsigned int num_unocc = num_orb - num_elec / 2;
    return num_elec * (num_elec / 2 - 1) * num_unocc * (num_unocc - 1) / 2 +
    num_elec / 2 * num_elec / 2 * num_unocc * num_unocc;
}


size_t gen_hf_ex(long long hf_det, unsigned char *hf_occ, unsigned int num_elec,
                 unsigned int n_orb, unsigned char *orb_symm, double (*eris)[n_orb][n_orb][n_orb],
                 unsigned int n_frozen, long long *ex_dets, double *ex_mel) {
    unsigned int num_unf_orb = n_orb - n_frozen / 2;
    size_t max_n_doub = count_doub_nosymm(num_elec, num_unf_orb);
    unsigned char ex_arr[max_n_doub][4];
    size_t num_hf_doub = doub_ex_symm(hf_det, hf_occ, num_elec, num_unf_orb, ex_arr, orb_symm);
    size_t idx;
    double matr_el;
    long long new_det;
    for (idx = 0; idx < num_hf_doub; idx++) {
        new_det = hf_det;
        //        matr_el = doub_matr_el_nosgn(&ex_arr[idx][0], eris, n_frozen, n_orb);
        matr_el = doub_matr_el_nosgn(&ex_arr[idx][0], n_orb, eris, n_frozen);
        matr_el *= doub_det_parity(&new_det, &ex_arr[idx][0]);
        ex_dets[idx] = new_det;
        ex_mel[idx] = matr_el;
    }
    return num_hf_doub;
}

size_t count_singex(long long det, unsigned char *occ_orbs, unsigned char *orb_symm,
                    unsigned int num_orb, unsigned char (* lookup_tabl)[num_orb + 1],
                    unsigned int num_elec) {
    size_t num_ex = 0;
    unsigned int elec_idx, symm_idx;
    unsigned char elec_symm, elec_orb;
    int elec_spin;
    for (elec_idx = 0; elec_idx < num_elec; elec_idx++) {
        elec_orb = occ_orbs[elec_idx];
        elec_symm = orb_symm[elec_orb % num_orb];
        elec_spin = elec_orb / num_orb;
        for (symm_idx = 0; symm_idx < lookup_tabl[elec_symm][0]; symm_idx++) {
            if (!(det & (1LL << (lookup_tabl[elec_symm][symm_idx + 1] +
                                 num_orb * elec_spin))))
            num_ex += 1;
        }
    }
    return num_ex;
}

double diag_matrel(unsigned char *occ_orbs, unsigned int n_orbs,
                   double (* eris)[n_orbs][n_orbs][n_orbs], double (* h_core)[n_orbs],
                   unsigned int n_frozen, unsigned int n_elec) {
    unsigned int j, k, elec_1, elec_2;
    double matr_sum = 0;
    unsigned int n_e_unf = n_elec - n_frozen;
    
    for (j = 0; j < n_frozen / 2; j++) {
        matr_sum += h_core[j][j] * 2;
        matr_sum += eris[j][j][j][j];
        for (k = j + 1; k < n_frozen / 2; k++) {
            matr_sum += eris[j][k][j][k] * 4;
            matr_sum -= eris[j][k][k][j] * 2;
        }
    }
    for (j = 0; j < n_e_unf / 2; j ++) {
        elec_1 = occ_orbs[j] + n_frozen / 2;
        matr_sum += h_core[elec_1][elec_1];
        for (k = 0; k < n_frozen / 2; k++) {
            matr_sum += eris[elec_1][k][elec_1][k] * 2;
            matr_sum -= eris[elec_1][k][k][elec_1];
        }
        for (k = j + 1; k < n_e_unf / 2; k++) {
            elec_2 = occ_orbs[k] + n_frozen / 2;
            matr_sum += eris[elec_1][elec_2][elec_1][elec_2];
            matr_sum -= eris[elec_1][elec_2][elec_2][elec_1];
        }
        for (k = n_e_unf / 2; k <  n_e_unf; k++) {
            elec_2 = occ_orbs[k] + n_frozen - n_orbs;
            matr_sum += eris[elec_1][elec_2][elec_1][elec_2];
        }
    }
    for (j = n_e_unf / 2; j < n_e_unf; j++) {
        elec_1 = occ_orbs[j] + n_frozen - n_orbs;
        matr_sum += h_core[elec_1][elec_1];
        for (k = 0; k < n_frozen / 2; k++) {
            matr_sum += eris[elec_1][k][elec_1][k] * 2;
            matr_sum -= eris[elec_1][k][k][elec_1];
        }
        for (k = j + 1; k < n_e_unf; k++) {
            elec_2 = occ_orbs[k] + n_frozen - n_orbs;
            matr_sum += eris[elec_1][elec_2][elec_1][elec_2];
            matr_sum -= eris[elec_1][elec_2][elec_2][elec_1];
        }
    }
    return matr_sum;
}