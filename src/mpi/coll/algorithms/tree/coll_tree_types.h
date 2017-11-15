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

#ifndef MPIR_COLL_TREE_TYPES_H
#define MPIR_COLL_TREE_TYPES_H

#include "coll_args_types.h"

typedef struct {
    int rank;
    int nranks;
    int parent;
    int num_children;
    int max_children;
    int *children;
} MPIR_COLL_tree_t;

typedef struct {
    int coll_op;
    union { /* Most of the structures below are empty, they will be populated as
               the corresponding collectives are implemented */
        /* Blocking collectives */
        struct {
            MPIR_COLL_bcast_args_t bcast;
            int tree_type;
            int k;
            int segsize;
        } bcast;
        struct {
        } reduce;
        struct {
        } allreduce;
        struct {
        } barrier;
        struct {
        } scatter;
        struct {
        } gather;
        struct {
        } allgather;
        struct {
        } alltoall;
        struct {
        } alltoallv;
        struct {
        } alltoallw;
        struct {
        } reducescatter;
        struct {
        } scan;
        struct {
        } exscan;
        struct {
        } gatherv;
        struct {
        } allgatherv;
        struct {
        } scatterv;
        /* Non-blocking collectives */
        struct {
        } iallgather;
        struct {
        } iallgatherv;
        struct {
        } iallreduce;
        struct {
        } ialltoall;
        struct {
        } ialltoallv;
        struct {
        } ialltoallw;
        struct {
        } ibarrier;
        struct {
        } ibcast;
        struct {
        } iexscan;
        struct {
        } igather;
        struct {
        } igatherv;
        struct {
        } ired_scat;
        struct {
        } ired_scat_block;
        struct {
        } ireduce;
        struct {
        } iscan;
        struct {
        } iscatter;
        struct {
        } iscatterv;
        /* Blocking neighborhood collectives */
        struct {
        } nhb_allgather;
        struct {
        } nhb_allgatherv;
        struct {
        } nhb_alltoall;
        struct {
        } nhb_alltoallv;
        struct {
        } nhb_alltoallw;
        /* Non-blocking neighborhood collectives */
        struct {
        } inhb_allgather;
        struct {
        } inhb_allgatherv;
        struct {
        } inhb_alltoall;
        struct {
        } inhb_alltoallv;
        struct {
        } inhb_alltoallw;
    } args;
} MPIR_COLL_tree_args_t;        /* structure used as key for schedule database */

#endif /* MPIR_COLL_TREE_TYPES_H */
