/*! \file
 *
 * \brief Utilities for reading/writing data from/to disk.
 */

#include "io_utils.hpp"

size_t read_doub_csv(double *buf, char *fname) {
    int i =  0;
    size_t row_idx = 0;
    size_t n_col;
    size_t n_read = 0;
    //                                   file, delimiter, first_line_is_header?
    CsvParser *csvparser = CsvParser_new(fname, ",", 0);
    CsvRow *row;
    
    while ((row = CsvParser_getRow(csvparser)) ) {
        const char **rowFields = CsvParser_getFields(row);
        n_col = CsvParser_getNumFields(row);
        n_read += n_col;
        for (i = 0 ; i < n_col ; i++) {
            sscanf(rowFields[i], "%lf", &buf[n_col * row_idx + i]);
        }
        CsvParser_destroy_row(row);
        row_idx++;
    }
    CsvParser_destroy(csvparser);
    return n_read;
}

size_t read_uchar_csv(uint8_t *buf, char *fname) {
    int i =  0;
    size_t row_idx = 0;
    size_t n_col;
    size_t n_read = 0;
    //                                   file, delimiter, first_line_is_header?
    CsvParser *csvparser = CsvParser_new(fname, " ", 0);
    CsvRow *row;
    unsigned int scan_val;
    
    while ((row = CsvParser_getRow(csvparser)) ) {
        const char **rowFields = CsvParser_getFields(row);
        n_col = CsvParser_getNumFields(row);
        n_read += n_col;
        for (i = 0 ; i < n_col ; i++) {
            sscanf(rowFields[i], "%u", &scan_val);
            buf[n_col * row_idx + i] = scan_val;
        }
        CsvParser_destroy_row(row);
        row_idx++;
    }
    CsvParser_destroy(csvparser);
    return n_read;
}


int parse_hf_input(const char *hf_dir, hf_input *in_struct) {
    char buffer[200];
    strcpy(buffer, hf_dir);
    strcat(buffer, "sys_params.txt");
    FILE *file_p = fopen(buffer, "r");
    if (!file_p) {
        fprintf(stderr, "Error: could not open file sys_params.txt\n");
        return -1;
    }
    
    char *str_p = fgets(buffer, sizeof(buffer), file_p);
    int success = strcmp(buffer, "n_elec");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%u", &(in_struct->n_elec));
    }
    else {
        fprintf(stderr, "Error: could not find n_elec parameter in sys_params.txt\n");
        return -1;
    }
    
    str_p = fgets(buffer, sizeof(buffer), file_p);
    success = strcmp(buffer, "n_frozen");
    unsigned int n_frz;
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%u", &n_frz);
    }
    else {
        fprintf(stderr, "Error: could not find n_frozen parameter in sys_params.txt\n");
        return -1;
    }
    in_struct->n_frz = n_frz;
    
    str_p = fgets(buffer, sizeof(buffer), file_p);
    success = strcmp(buffer, "n_orb");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%u", &(in_struct->n_orb));
    }
    else {
        fprintf(stderr, "Error: could not find n_orb parameter in sys_params.txt\n");
        return -1;
    }
    
    str_p = fgets(buffer, sizeof(buffer), file_p);
    success = strcmp(buffer, "eps");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%lf", &(in_struct->eps));
    }
    else {
        fprintf(stderr, "Error: could not find eps parameter in sys_params.txt\n");
        return -1;
    }
    
    str_p = fgets(buffer, sizeof(buffer), file_p);
    success = strcmp(buffer, "hf_energy");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%lf", &(in_struct->hf_en));
    }
    else {
        fprintf(stderr, "Error: could not find hf_energy parameter in sys_params.txt\n");
        return -1;
    }
    
    fclose(file_p);
    
    unsigned int tot_orb = in_struct->n_orb + n_frz / 2;
    strcpy(buffer, hf_dir);
    strcat(buffer, "symm.txt");
    in_struct->symm = (uint8_t *)malloc(sizeof(uint8_t) * tot_orb);
    read_uchar_csv(in_struct->symm, buffer);
    in_struct->symm = &(in_struct->symm[n_frz / 2]);
    
    strcpy(buffer, hf_dir);
    strcat(buffer, "hcore.txt");
    in_struct->hcore = new Matrix<double>(tot_orb, tot_orb);
    size_t n_read = read_doub_csv((*(in_struct->hcore))[0], buffer);
    if (n_read < tot_orb * tot_orb) {
        fprintf(stderr, "Error reading values from %s\n", buffer);
        return -1;
    }
    
    strcpy(buffer, hf_dir);
    strcat(buffer, "eris.txt");
    in_struct->eris = new FourDArr(tot_orb, tot_orb, tot_orb, tot_orb);
    n_read = read_doub_csv(in_struct->eris->data(), buffer);
    if (n_read < tot_orb * tot_orb * tot_orb * tot_orb) {
        fprintf(stderr, "Error reading values from %s\n", buffer);
        return -1;
    }
    
    return 0;
}

int parse_hh_input(const char *hh_path, hh_input *in_struct) {
    FILE *file_p = fopen(hh_path, "r");
    if (!file_p) {
        fprintf(stderr, "Error: could not open file containing Hubbard-Holstein parameters\n");
        return -1;
    }
    
    char buffer[100];
    char *str_p = fgets(buffer, sizeof(buffer), file_p);
    int success = strcmp(buffer, "n_elec");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%u", &(in_struct->n_elec));
    }
    else {
        fprintf(stderr, "Error: could not find n_elec parameter in %s\n", hh_path);
        return -1;
    }
    
    str_p = fgets(buffer, sizeof(buffer), file_p);
    success = strcmp(buffer, "lat_len");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%u", &(in_struct->lat_len));
    }
    else {
        fprintf(stderr, "Error: could not find lat_len parameter in %s\n", hh_path);
        return -1;
    }
    
    str_p = fgets(buffer, sizeof(buffer), file_p);
    success = strcmp(buffer, "n_dim");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%u", &(in_struct->n_dim));
    }
    else {
        fprintf(stderr, "Error: could not find n_dim parameter in %s\n", hh_path);
        return -1;
    }
    
    str_p = fgets(buffer, sizeof(buffer), file_p);
    success = strcmp(buffer, "eps");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%lf", &(in_struct->eps));
    }
    else {
        fprintf(stderr, "Error: could not find eps parameter in %s\n", hh_path);
        return -1;
    }
    
    str_p = fgets(buffer, sizeof(buffer), file_p);
    success = strcmp(buffer, "U");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%lf", &(in_struct->elec_int));
    }
    else {
        fprintf(stderr, "Error: could not find electron interaction parameter (U) in %s\n", hh_path);
        return -1;
    }
    
    str_p = fgets(buffer, sizeof(buffer), file_p);
    success = strcmp(buffer, "hf_energy");
    if (success) {
        str_p = fgets(buffer, sizeof(buffer), file_p);
        success = !(!str_p);
    }
    if (success) {
        sscanf(buffer, "%lf", &(in_struct->hf_en));
    }
    else {
        fprintf(stderr, "Error: could not find hf_energy parameter in %s\n", hh_path);
        return -1;
    }
    
    fclose(file_p);
    return 0;
}

size_t load_vec_txt(const char *prefix, Matrix<uint8_t> &dets, void *vals, dtype type) {
    int my_rank = 0;
#ifdef USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
#endif
    
    if (my_rank == 0) {
        char buffer[100];
        sprintf(buffer, "%sdets", prefix);
        FILE *file_d = fopen(buffer, "r");
        if (!file_d) {
            fprintf(stderr, "Warning: could not find file: %s\n", buffer);
        }
        sprintf(buffer, "%svals", prefix);
        FILE *file_v = fopen(buffer, "r");
        if (!file_v) {
            fprintf(stderr, "Warning: could not find file: %s\n", buffer);
        }
        if (!file_d || !file_v) {
            return 0;
        }
        int num_read_d = 1;
        int num_read_v = 1;
        size_t n_dets = 0;
        long long in_det;
        size_t byte_idx;
        size_t max_size = dets.cols();
        
        if (type == DOUB) {
            double *val_arr = (double *)vals;
            while (num_read_d == 1 && num_read_v == 1) {
                num_read_d = fscanf(file_d, "%lld\n", &in_det);
                num_read_v = fscanf(file_v, "%lf\n", &val_arr[n_dets]);
                for (byte_idx = 0; byte_idx < 8 && byte_idx < max_size; byte_idx++) {
                    dets(n_dets, byte_idx) = in_det & 255;
                    in_det >>= 8;
                }
                n_dets++;
            }
        }
        else if (type == INT) {
            int *val_arr = (int *) vals;
            while (num_read_d == 1 && num_read_v == 1) {
                num_read_d = fscanf(file_d, "%lld\n", &in_det);
                num_read_v = fscanf(file_v, "%d\n", &val_arr[n_dets]);
                for (byte_idx = 0; byte_idx < 8 && byte_idx < max_size; byte_idx++) {
                    dets(n_dets, byte_idx) = in_det & 255;
                    in_det >>= 8;
                }
                n_dets++;
            }
        }
        else {
            fprintf(stderr, "Error: data type %d not supported in function load_vec_txt.\n", type);
        }
        return --n_dets;
    }
    else {
        return 0;
    }
}


void save_proc_hash(const char *path, unsigned int *proc_hash, size_t n_hash) {
    int my_rank = 0;
#ifdef USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
#endif
    
    char buffer[100];
    if (my_rank == 0) {
        sprintf(buffer, "%shash.dat", path);
        FILE *file_p = fopen(buffer, "wb");
        if (!file_p) {
            fprintf(stderr, "Error: could not save file at path %s\n", buffer);
            return;
        }
        fwrite(proc_hash, sizeof(unsigned int), n_hash, file_p);
        fclose(file_p);
    }
}


void load_proc_hash(const char *path, unsigned int *proc_hash) {
    char buffer[100];
    sprintf(buffer, "%shash.dat", path);
    FILE *file_p = fopen(buffer, "rb");
    if (!file_p) {
        fprintf(stderr, "Error: could not open saved hash scrambler at %s\n", buffer);
        return;
    }
    fread(proc_hash, sizeof(unsigned int), 1000, file_p);
    fclose(file_p);
}

