//
//  hub_holstein.h
//  FRIes
//
//  Created by Samuel Greene on 7/15/19.
//  Copyright © 2019 Samuel Greene. All rights reserved.
//

#ifndef hub_holstein_h
#define hub_holstein_h

#include <stdio.h>
#include "fci_utils.h"
#include "math_utils.h"
#include "dc.h"

/* Uniformly chooses num_sampl excitations from a determinant
 using independent multinomial sampling.
 
 Arguments
 ---------
 det: bit string representation of the origin determinant
 n_elec: number of electrons in the system
 neighbors: list of orbitals with unoccupied neighbors, generated by find_neighbors()
 num_sampl: number of excitations to choose
 rn_ptr: pointer to rn generator
 chosen_orbs: array that, upon return, contains occupied and
    unoccupied orbitals for each excitation
 */
void hub_multin(long long det, unsigned int n_elec, unsigned char (*neighbors)[n_elec + 1],
                unsigned int num_sampl, mt_struct *rn_ptr, unsigned char (* chosen_orbs)[2]);

/* Generate all allowed excitations from a determinant
 
 Argumemnts
 ----------
 det: bit string representation of the origin determinant
 n_elec: number of electrons in the system
 neighbors: list of orbitals with unoccupied neighbors, generated by find_neighbors()
 num_sampl: number of excitations to choose
 rn_ptr: pointer to rn generator
 chosen_orbs: array that, upon return, contains occupied and
 unoccupied orbitals for each excitation
 
 Returns
 -------
 number of excitations
 */
size_t hub_all(long long det, unsigned int n_elec, unsigned char (*neighbors)[n_elec + 1],
               unsigned char (* chosen_orbs)[2]);


/*
 Generate lists of orbitals in a determinant that are adjacent (in a bit string
 representation) to an empty orbital.
 
 Arguments
 ---------
 det: bit string to be parsed
 n_sites: number of sites along one dimension of the Hubbard lattice
 table: byte table structure generated by gen_byte_table()
 n_elec: number of electrons in the system
 neighbors: 2-D array that contains results upon return. 0th row indicates orbitals
    with an empty adjacent orbital to the left, and 1st row is for empty orbitals
    to the right. Elements in the 0th column indicate number of elements in each
    row.
 */
void find_neighbors(long long det, unsigned int n_sites, byte_table *table,
                    unsigned int n_elec, unsigned char (*neighbors)[n_elec + 1]);

/*
 Find the diagonal matrix element for a determinant, in units of U
 
 Arguments
 ---------
 det: bit string representation of the determinant
 n_sites: number of sites along one dimension of the Hubbard lattice
 table: byte table structure generated by gen_byte_table()
 
 Returns
 -------
 number of spin-up & spin-down orbitals that overlap
 */
unsigned int hub_diag(long long det, unsigned int n_sites, byte_table *table);


/* Generate the bit string for the Neel state in the Hubbard model.
 
 Arguments
 ---------
 n_sites: number of sites along one dimension of Hubbard model
 n_elec: the number of electrons in the system
 n_dim: number of dimensions in the Hubbard model
 
 Returns: bit string representation of the determinant
 */
long long gen_hub_bitstring(unsigned int n_sites, unsigned int n_elec, unsigned int n_dim);


/*
 Identify all Slater determinants in a vector connected via H to a reference
 determinant and sum their matrix elements
 
 dets: slater determinant indices of vector elements
 vals: values of vector elements
 n_dets: number of elements in the sparse vector representation
 ref_det: reference determinant to which H is applied
 
 Returns
 -------
 sum of all (off-diagonal) matrix elements
 */
double calc_ref_ovlp(long long *dets, void *vals, size_t n_dets, long long ref_det,
                     byte_table *table, dtype type);

#endif /* hub_holstein_h */
