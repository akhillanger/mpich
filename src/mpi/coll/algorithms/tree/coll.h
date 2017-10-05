/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include <stdio.h>
#include <string.h>
#include "queue.h"

#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif

#include "coll_progress_util.h"
#include "coll_schedule_tree.h"

/* Initialize collective algorithm */
MPIC_INLINE int COLL_init()
{
    return 0;
}

/* Initialize algorithm specific communicator */
MPIC_INLINE int COLL_comm_init(COLL_comm_t * comm, int *tag_ptr, int rank, int comm_size)
{
    int k;

    /* initialize transport communicator */
    TSP_comm_init(&comm->tsp_comm, COLL_COMM_BASE(comm));

    /* generate a knomial tree (tree_type=0) by default */
    COLL_tree_comm_t *mycomm = &comm->tree_comm;
    k = COLL_TREE_RADIX_DEFAULT;

    COLL_tree_init(rank, comm_size, 0, k, 0, &mycomm->tree);

    comm->tree_comm.curTag = tag_ptr;
    return 0;
}

/* Cleanup algorithm communicator */
MPIC_INLINE int COLL_comm_cleanup(COLL_comm_t * comm)
{
    TSP_comm_cleanup(&comm->tsp_comm);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME COLL_allreduce
MPIC_INLINE int COLL_allreduce(const void *sendbuf,
                               void *recvbuf,
                               int count,
                               COLL_dt_t datatype,
                               COLL_op_t op, COLL_comm_t * comm, int *errflag, int tree_type, int k)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_TREE_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_TREE_ALLREDUCE);

    int mpi_errno = MPI_SUCCESS;
    /* generate key for searching the schedule database */
    COLL_args_t coll_args = {.coll_op = ALLREDUCE,
        .nargs = 10,
        .args = {.allreduce = {.sbuf = (void *) sendbuf,
                               .rbuf = recvbuf,
                               .count = count,
                               .dt_id = (int) datatype,
                               .op_id = (int) op,
                               .tree_type = tree_type,
                               .k = k, .pad1=0, .pad2=0, .pad3=0}}
    };

    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;
    MPIC_DBG("In COLL_allred - comm:%p, sendbuf:%p, recvbuf:%p, count:%d, datatype:%d, \
		op:%d, tree_type:%d, k:%d\n", comm, sendbuf, recvbuf, count, datatype, op, tree_type, k);
    /* Check with the transport if schedule already exisits
     * If it exists, reset and return the schedule else
     * return a new empty schedule */
    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm, (void *) &coll_args,
                                      sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {       /* schedule does not exist, needs to be generated */
        MPIC_DBG("Schedule does not exist\n");
        /* schedule tree based allreduce */
        mpi_errno = COLL_sched_allreduce_tree(sendbuf, recvbuf, count,
                                              datatype, op, tag, comm, tree_type, k, s, 1);
        /* save the schedule */
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }
    /* execute the schedule */
    COLL_kick_sched(s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_TREE_ALLREDUCE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME COLL_bcast_get_tree_schedule
/* Internal function that returns broadcast schedule. This is used by COLL_bcast and COLL_ibcast
 * to get the broadcast schedule */
MPIC_INLINE int COLL_bcast_get_tree_schedule(void *buffer, int count, COLL_dt_t datatype,
                                                      int root, COLL_comm_t * comm, int tree_type,
                                                      int k, int segsize, TSP_sched_t ** sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_BCAST_GET_TREE_SCHEDULE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_BCAST_GET_TREE_SCHEDULE);

    int mpi_errno = MPI_SUCCESS;

    /* generate key for searching the schedule database */
    COLL_args_t coll_args = {.coll_op = BCAST,
        .nargs = 7,
        .args = {.bcast = {.buf = buffer,
                           .count = count,
                           .dt_id = (int) datatype,
                           .root = root,
                           .tree_type = tree_type,
                           .k = k,
                           .segsize = segsize}}
    };

    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;

    /* Check with the transport if schedule already exisits
     * If it exists, reset and return the schedule else
     * return a new empty schedule */
    *sched =
        TSP_get_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        MPIC_DBG("Schedule does not exist\n");
        /* generate the schedule */
        mpi_errno = COLL_sched_bcast_tree_pipelined(buffer, count, datatype, root, tag, comm,
                                                    tree_type, k, segsize, *sched, 1);
        /* store the schedule (it is optional for the transport to store the schedule */
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t),
                          (void *) *sched);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_BCAST_GET_TREE_SCHEDULE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME COLL_bcast
/* Blocking broadcast */
MPIC_INLINE int COLL_bcast(void *buffer, int count, COLL_dt_t datatype, int root,
                           COLL_comm_t * comm, int *errflag, int tree_type, int k, int segsize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_TREE_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_TREE_BCAST);

    int mpi_errno = MPI_SUCCESS;

    /* get the schedule */
    TSP_sched_t *sched;
    mpi_errno = COLL_bcast_get_tree_schedule(buffer, count, datatype, root, comm, tree_type, k, segsize, &sched);

    /* execute the schedule */
    COLL_kick_sched(sched);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_TREE_BCAST);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME COLL_ibcast
/* Non-blocking broadcast */
MPIC_INLINE int COLL_ibcast(void *buffer, int count, COLL_dt_t datatype, int root,
                            COLL_comm_t * comm, COLL_req_t * request, int tree_type, int k,
                            int segsize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_TREE_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_TREE_IBCAST);

    int mpi_errno = MPI_SUCCESS;

    /* get the schedule */
    TSP_sched_t *sched;
    mpi_errno = COLL_bcast_get_tree_schedule(buffer, count, datatype, root, comm, tree_type, k, segsize, &sched);

    /* Enqueue schedule to non-blocking collectives queue, and start it */
    COLL_kick_sched_nb(sched, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_TREE_IBCAST);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME COLL_reduce
MPIC_INLINE int COLL_reduce(const void *sendbuf,
                            void *recvbuf,
                            int count,
                            COLL_dt_t datatype,
                            COLL_op_t op, int root, COLL_comm_t * comm, int *errflag, int tree_type,
                            int k, int segsize, int nbuffers)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_TREE_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_TREE_REDUCE);

    int mpi_errno = MPI_SUCCESS;
    /* generate the key for searching the schedule database */
    COLL_args_t coll_args = {.coll_op = REDUCE,
        .nargs = 10,
        .args = {.reduce = {.sbuf = (void *) sendbuf,
                            .rbuf = recvbuf,
                            .count = count,
                            .dt_id = (int) datatype,
                            .op_id = (int) op,
                            .tree_type = tree_type,
                            .k = k,
                            .segsize = segsize,
                            .nbuffers = nbuffers,
                            .root = root}}
    };


    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;

    /* Check with the transport if schedule already exisits
     * If it exists, reset and return the schedule else
     * return a new empty schedule */
    TSP_sched_t *s =
        TSP_get_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {       /* schedule is new */
        /* generate the schedule */
        mpi_errno =
            COLL_sched_reduce_tree_full_pipelined(sendbuf, recvbuf, count, datatype, op, root, tag,
                                                  comm, tree_type, k, s, segsize, 0, nbuffers);
        /* save the schedule */
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }

    /* execute the schedule */
    COLL_kick_sched(s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_TREE_REDUCE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME COLL_barrier
MPIC_INLINE int COLL_barrier(COLL_comm_t * comm, int *errflag, int tree_type, int k)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_TREE_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_TREE_BARRIER);

    int mpi_errno = MPI_SUCCESS;
    /* generate the key for the schedule */
    COLL_args_t coll_args = {.coll_op = BARRIER,
        .nargs = 2,
        .args = {.barrier = {.tree_type = tree_type,.k = k}}
    };

    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;

    /* Check with the transport if schedule already exisits
     * If it exists, reset and return the schedule else
     * return a new empty schedule */
    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm,
                                      (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        /* generate the schedule */
        mpi_errno = COLL_sched_barrier_tree(tag, comm, tree_type, k, s);

        /* save the schedule */
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }

    /* execute the schedule */
    COLL_kick_sched(s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_TREE_BARRIER);

    return mpi_errno;
}