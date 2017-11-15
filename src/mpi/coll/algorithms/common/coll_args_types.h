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

#ifndef MPIR_COLL_ARGS_GENERIC_H
#define MPIR_COLL_ARGS_GENERIC_H

typedef struct {
} MPIR_COLL_barrier_args_t;     /* barrier had no arguments */

typedef struct {
    void *buf;
    int count;
    int dt_id;
    int root;
    int pad;
} MPIR_COLL_bcast_args_t;

typedef struct {
    const void *sbuf;
    void *rbuf;
    int count;
    int dt_id;
    int op_id;
    int root;
} MPIR_COLL_reduce_args_t;

typedef struct {
    const void *sbuf;
    void *rbuf;
    int count;
    int dt_id;
    int op_id;
    int pad;
} MPIR_COLL_allreduce_args_t;

typedef struct {
    const void *sbuf;
    int scount;
    int st_id;
    void *rbuf;
    int rcount;
    int rt_id;
} MPIR_COLL_alltoall_args_t;
#endif /* MPIR_COLL_ARGS_GENERIC_H */
