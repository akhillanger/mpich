/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

typedef TSP_dt_t COLL_dt_t;
typedef TSP_op_t COLL_op_t;

typedef struct COLL_comm_t {
    TSP_comm_t tsp_comm;
    int *curTag;                /*tag for collective operations */
    int id;                     /*unique id for this communicator */
} COLL_comm_t;

typedef MPIC_req_t COLL_req_t;

typedef TSP_aint_t COLL_aint_t;

typedef struct COLL_global_t {
} COLL_global_t;

typedef struct {
    int algo;                   /*algorithm type */
    int tsp;                    /*transport type */
    int nargs;                  /*number of arguments */
    union {
        struct {
            void *buf;
            int count;
            int dt_id;
            int root;
            int comm_id;
        } bcast;
        struct {
            void *sbuf;
            void *rbuf;
            int count;
            int dt_id;
            int op_id;
            int root;
            int comm_id;
        } reduce;
        struct {
            void *sbuf;
            void *rbuf;
            int count;
            int dt_id;
            int op_id;
            int comm_id;
        } allreduce;
        struct {
            int k;
        } barrier;
    } args;
} COLL_args_t;
