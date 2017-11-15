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
#ifndef MPIR_COLL_TYPES_H_INCLUDED
#define MPIR_COLL_TYPES_H_INCLUDED

#include "queue.h"
#include "uthash.h"

/* include types from transports */
#include "stub_tsp_types.h"
#include "generic_tsp_types.h"

/* Type definitions from all algorithms */
#include "coll_stub_types.h"
#include "coll_tree_types.h"

#define MPIR_COLL_FLAG_REDUCE_L 1
#define MPIR_COLL_FLAG_REDUCE_R 0

/* enumerator for different tree types */
enum {
    TREE_TYPE_KNOMIAL = 0,
    TREE_TYPE_KARY
};

/* enumerator for all the collective operations */
enum {
    /* Blocking collectives */
    ALLGATHER = 0,
    ALLGATHERV,
    ALLREDUCE,
    ALLTOALL,
    ALLTOALLV,
    ALLTOALLW,
    BARRIER,
    BCAST,
    EXSCAN,
    GATHER,
    GATHERV,
    RED_SCAT,
    RED_SCAT_BLOCK,
    REDUCE,
    SCAN,
    SCATTER,
    SCATTERV,
    /* Non blocking collectives */
    IALLGATHER,
    IALLGATHERV,
    IALLREDUCE,
    IALLTOALL,
    IALLTOALLV,
    IALLTOALLW,
    IBARRIER,
    IBCAST,
    IEXSCAN,
    IGATHER,
    IGATHERV,
    IRED_SCAT,
    IRED_SCAT_BLOCK,
    IREDUCE,
    ISCAN,
    ISCATTER,
    ISCATTERV,
    /* Blocking neighborhood collectives */
    NHB_ALLGATHER,
    NHB_ALLGATHERV,
    NHB_ALLTOALL,
    NHB_ALLTOALLV,
    NHB_ALLTOALLW,
    /*Non-blocking neighborhood collectives */
    INHB_ALLGATHER,
    INHB_ALLGATHERV,
    INHB_ALLTOALL,
    INHB_ALLTOALLV,
    INHB_ALLTOALLW
};

/* schedule entry in the database */
typedef struct sched_entry {
    void *sched;                /* pointer to the schedule */
    UT_hash_handle handle;      /* hash handle that makes this structure hashable */
    int size;                   /* Storage for variable-len key */
    char arg[0];
} MPIR_COLL_sched_entry_t;

/* function pointer to free a schedule */
typedef void (*MPIR_COLL_sched_free_fn) (void *);

/* Collectives request data structure */
typedef struct MPIR_COLL_req_t {
    TAILQ_ENTRY(MPIR_COLL_req_t) list_data;     /* this structure can also be a queue element */
    void *sched;                /* pointer to the schedule */
} MPIR_COLL_req_t;

typedef struct {
    TAILQ_HEAD(MPIR_COLL_tail_queue_head, MPIR_COLL_req_t) head;
} MPIR_COLL_queue_t;

/* collective algorithm communicators */
typedef struct {
    int tag;
    void *sched_db;
} MPIR_COLL_comm_t;

#endif /* MPIR_COLL_TYPES_H_INCLUDED */
