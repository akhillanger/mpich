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

#include "coll_pipeline_util.h"
#include "coll_tree_util.h"

#ifndef MPIR_TSP_NAMESPACE
#error "The tree template must be namespaced with MPIR_TSP_NAMESPACE"
#endif

#undef FUNCNAME
#define FUNCNAME MPIR_TSP_sched_bcast_tree
/* Routine to schedule a tree based broadcast */
MPL_STATIC_INLINE_PREFIX int
MPIR_TSP_sched_bcast_tree(void *buffer, int count, MPI_Datatype datatype, int root, int tag,
                          MPIR_Comm * comm, int tree_type, int k, MPIR_TSP_sched_t * sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_SCHED_BCAST_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_SCHED_BCAST_TREE);

    int mpi_errno = MPI_SUCCESS;
    int size = MPIR_Comm_size(comm);
    int rank = MPIR_Comm_rank(comm);
    int recv_id;
    MPIR_COLL_tree_t my_tree;

    my_tree.children = NULL;
    mpi_errno = MPIR_COLL_init_tree(rank, size, tree_type, k, root, &my_tree);

    /* Receive message from Parent */
    if (my_tree.parent != -1) {
        recv_id = MPIR_TSP_sched_recv(buffer, count, datatype, my_tree.parent, tag,
                                      comm, sched, 0, NULL);
    }

    int num_children = my_tree.num_children;

    if (num_children) {
        /*Multicast data to the children */
        MPIR_TSP_sched_multicast(buffer, count, datatype, my_tree.children, num_children, tag, comm,
                                 sched, (my_tree.parent != -1) ? 1 : 0, &recv_id);
    }

    MPIR_COLL_free_tree(&my_tree);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_SCHED_BCAST_TREE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_TSP_sched_bcast_tree_pipelined
/* Routine to schedule a pipelined tree based broadcast */
MPL_STATIC_INLINE_PREFIX int
MPIR_TSP_sched_bcast_tree_pipelined(void *buffer, int count, MPI_Datatype datatype, int root,
                                    int tag, MPIR_Comm * comm, int tree_type, int k, int segsize,
                                    MPIR_TSP_sched_t * sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_SCHED_BCAST_TREE_PIPELINED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_SCHED_BCAST_TREE_PIPELINED);

    int mpi_errno = MPI_SUCCESS;
    int i;

    /* variables to store pipelining information */
    int num_chunks, num_chunks_floor, chunk_size_floor, chunk_size_ceil;
    int offset = 0;

    /* variables for storing datatype information */
    size_t extent, type_size;
    MPI_Aint lb, true_extent;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Scheduling pipelined broadcast on %d ranks, root=%d\n",
                     MPIR_Comm_size(comm), root));

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    /* calculate chunking information for pipelining */
    MPIR_COLL_calculate_chunk_info(segsize, type_size, count, &num_chunks, &num_chunks_floor,
                                   &chunk_size_floor, &chunk_size_ceil);
    /* print chunking information */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                             "Broadcast pipeline info: segsize=%d count=%d num_chunks=%d num_chunks_floor=%d chunk_size_floor=%d chunk_size_ceil=%d \n",
                                             segsize, count, num_chunks, num_chunks_floor,
                                             chunk_size_floor, chunk_size_ceil));

    /* do pipelined broadcast */
    /* NOTE: Make sure you are handling non-contiguous datatypes correctly with pipelined
     * broadcast, for example, buffer+offset if being calculated correctly */
    for (i = 0; i < num_chunks; i++) {
        int msgsize = (i < num_chunks_floor) ? chunk_size_floor : chunk_size_ceil;
        mpi_errno =
            MPIR_TSP_sched_bcast_tree((char *) buffer + offset * extent, msgsize, datatype, root,
                                      tag, comm, tree_type, k, sched);
        offset += msgsize;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_SCHED_BCAST_TREE_PIPELINED);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_TSP_get_bcast_tree_schedule
/* Internal function that returns broadcast schedule. This is used by MPIR_COLL_bcast and MPIR_COLL_ibcast
 * to get the broadcast schedule */
MPL_STATIC_INLINE_PREFIX
    int MPIR_TSP_get_bcast_tree_schedule(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, int tree_type,
                                         int k, int segsize, MPIR_TSP_sched_t ** sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_GET_BCAST_TREE_SCHEDULE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_GET_BCAST_TREE_SCHEDULE);

    int mpi_errno = MPI_SUCCESS;

    /* generate key for searching the schedule database */
    MPIR_COLL_tree_args_t coll_args = {.coll_op = BCAST,
        .args = {.bcast = {.bcast = {.buf = buffer,
                                     .count = count,
                                     .dt_id = (int) datatype,
                                     .root = root,
                                     .pad = 0},
                           .tree_type = tree_type,
                           .k = k,
                           .segsize = segsize}
                 }
    };

    int is_new = 0;
    int tag = (comm->coll.tag)++;

    /* Check with the transport if schedule already exisits
     * If it exists, reset and return the schedule else
     * return a new empty schedule */
    int key_size = MPIR_COLL_tree_get_key_size(&coll_args);
    *sched = MPIR_TSP_get_schedule(comm, (void *) &coll_args, key_size, tag, &is_new);

    if (is_new) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Schedule does not exist\n"));
        /* schedule pipelined tree algo */
        mpi_errno = MPIR_TSP_sched_bcast_tree_pipelined(buffer, count, datatype, root, tag, comm,
                                                        tree_type, k, segsize, *sched);
        /* schedule generation is complete */
        MPIR_TSP_sched_commit(*sched);

        /* store the schedule (it is optional for the transport to store the schedule */
        MPIR_TSP_save_schedule(comm, (void *) &coll_args, key_size, (void *) *sched);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_GET_BCAST_TREE_SCHEDULE);

    return mpi_errno;
}
