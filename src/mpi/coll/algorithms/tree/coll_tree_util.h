/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef COLL_TREE_UTIL_H
#define COLL_TREE_UTIL_H

/* Generate the tree */
int MPIR_COLL_init_tree(int rank, int nranks, int tree_type, int k, int root,
                        MPIR_COLL_tree_t * ct);

/* Free any memory associate with MPIR_COLL_tree_t */
void MPIR_COLL_free_tree(MPIR_COLL_tree_t * tree);

/* Utility routine to print a tree */
int MPIR_COLL_dump_tree(int tree_size, int root, int tree_type, int k);

/* utility function to add a child to MPIR_COLL_tree_t data structure */
void MPIR_COLL_add_child_tree(MPIR_COLL_tree_t * t, int rank);

/* Function to generate kary tree information for rank 'rank' and store it in MPIR_COLL_tree_t data structure */
void MPIR_COLL_init_kary_tree(int rank, int nranks, int k, int root, MPIR_COLL_tree_t * ct);

/* Function to generate knomial tree information for rank 'rank' and store it in MPIR_COLL_tree_t data structure */
void MPIR_COLL_init_knomial_tree(int rank, int nranks, int k, int root, MPIR_COLL_tree_t * ct);

#endif /* COLL_TREE_UTIL_H */
