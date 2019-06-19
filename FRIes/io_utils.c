//
//  io_utils.c
//  FRIes
//
//  Created by Samuel Greene on 3/30/19.
//  Copyright © 2019 Samuel Greene. All rights reserved.
//

#include "io_utils.h"

void read_in_doub(double *buf, char *fname) {
    int i =  0;
    size_t row_idx = 0;
    size_t n_col;
    //                                   file, delimiter, first_line_is_header?
    CsvParser *csvparser = CsvParser_new(fname, ",", 0);
    CsvRow *row;
    
    while ((row = CsvParser_getRow(csvparser)) ) {
        const char **rowFields = CsvParser_getFields(row);
        n_col = CsvParser_getNumFields(row);
        for (i = 0 ; i < n_col ; i++) {
            sscanf(rowFields[i], "%lf", &buf[n_col * row_idx + i]);
        }
        CsvParser_destroy_row(row);
        row_idx++;
    }
    CsvParser_destroy(csvparser);
}

void read_in_uchar(unsigned char *buf, char *fname) {
    int i =  0;
    size_t row_idx = 0;
    size_t n_col;
    //                                   file, delimiter, first_line_is_header?
    CsvParser *csvparser = CsvParser_new(fname, " ", 0);
    CsvRow *row;
    unsigned int scan_val;
    
    while ((row = CsvParser_getRow(csvparser)) ) {
        const char **rowFields = CsvParser_getFields(row);
        n_col = CsvParser_getNumFields(row);
        for (i = 0 ; i < n_col ; i++) {
            sscanf(rowFields[i], "%u", &scan_val);
            buf[n_col * row_idx + i] = scan_val;
        }
        CsvParser_destroy_row(row);
        row_idx++;
    }
    CsvParser_destroy(csvparser);
}


int parse_input(const char *hf_dir, hf_input *in_struct) {
    char buffer[100];
    strcpy(buffer, hf_dir);
    strcat(buffer, "sys_params.txt");
    FILE *file_p = fopen(buffer, "r");
    
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
    in_struct->symm = malloc(sizeof(unsigned char) * tot_orb);
    read_in_uchar(in_struct->symm, buffer);
    in_struct->symm = &(in_struct->symm[n_frz / 2]);
    
    strcpy(buffer, hf_dir);
    strcat(buffer, "hcore.txt");
    in_struct->hcore = malloc(sizeof(double) * tot_orb * tot_orb);
    read_in_doub(in_struct->hcore, buffer);
    
    strcpy(buffer, hf_dir);
    strcat(buffer, "eris.txt");
    in_struct->eris = malloc(sizeof(double) * tot_orb * tot_orb * tot_orb * tot_orb);
    read_in_doub(in_struct->eris, buffer);
    
    return 0;
}

void save_vec(const char *path, long long *dets, void *vals, size_t n_dets, size_t el_size) {
    int my_rank = 0;
#ifdef USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
#endif
    
    char buffer[100];
    sprintf(buffer, "%sdets%d.dat", path, my_rank);
    FILE *file_p = fopen(buffer, "wb");
    fwrite(dets, sizeof(long long), n_dets, file_p);
    fclose(file_p);
    
    sprintf(buffer, "%svals%d.dat", path, my_rank);
    file_p = fopen(buffer, "wb");
    fwrite(vals, el_size, n_dets, file_p);
    fclose(file_p);
}


size_t load_vec(const char *path, long long *dets, void *vals, size_t el_size) {
    int my_rank = 0;
#ifdef USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
#endif
    
    size_t n_dets;
    char buffer[100];
    sprintf(buffer, "%sdets%d.dat", path, my_rank);
    FILE *file_p = fopen(buffer, "rb");
    n_dets = fread(dets, sizeof(long long), 10000000, file_p);
    fclose(file_p);
    
    sprintf(buffer, "%svals%d.dat", path, my_rank);
    file_p = fopen(buffer, "rb");
    fread(vals, el_size, n_dets, file_p);
    fclose(file_p);
    
    return n_dets;
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
        fwrite(proc_hash, sizeof(unsigned int), n_hash, file_p);
        fclose(file_p);
    }
}


void load_proc_hash(const char *path, unsigned int *proc_hash) {
    char buffer[100];
    sprintf(buffer, "%shash.dat", path);
    FILE *file_p = fopen(buffer, "rrb");
    fread(proc_hash, sizeof(unsigned int), 1000, file_p);
    fclose(file_p);
}
