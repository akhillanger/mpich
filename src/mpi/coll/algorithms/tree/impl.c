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
#include "mpiimpl.h"

/* Initialize collective algorithm */
int MPIR_COLL_tree_init()
{
    return 0;
}

/* Initialize communicator data for tree algorithms */
int MPIR_COLL_tree_comm_init(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    /* currently, there is nothing to initialize */
    return mpi_errno;
}

/* Initialize communicator data for tree algorithms to NULL */
int MPIR_COLL_tree_comm_init_null(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    /* currently, there is nothing to initialize */
    return mpi_errno;
}

/* Cleanup any communicator data for tree algorithms */
int MPIR_COLL_tree_comm_cleanup(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    /* currently, there is nothing to cleanup */
    return mpi_errno;
}

int MPIR_COLL_tree_get_key_size(MPIR_COLL_tree_args_t * key)
{
    int key_size = 0;
    key_size += sizeof(key->coll_op);

    switch (key->coll_op) {
    case BCAST:
        key_size += sizeof(key->args.bcast);
        break;
    case REDUCE:
        key_size += sizeof(key->args.reduce);
        break;
    case ALLREDUCE:
        key_size += sizeof(key->args.allreduce);
        break;
    case SCATTER:
        key_size += sizeof(key->args.scatter);
        break;
    case GATHER:
        key_size += sizeof(key->args.gather);
        break;
    case ALLGATHER:
        key_size += sizeof(key->args.allgather);
        break;
    case ALLTOALL:
        key_size += sizeof(key->args.alltoall);
        break;
    case ALLTOALLV:
        key_size += sizeof(key->args.alltoallv);
        break;
    case ALLTOALLW:
        key_size += sizeof(key->args.alltoallw);
        break;
    case BARRIER:
        key_size += sizeof(key->args.barrier);
        break;
    case RED_SCAT:
        key_size += sizeof(key->args.reducescatter);
        break;
    case SCAN:
        key_size += sizeof(key->args.scan);
        break;
    case EXSCAN:
        key_size += sizeof(key->args.exscan);
        break;
    case GATHERV:
        key_size += sizeof(key->args.gatherv);
        break;
    case ALLGATHERV:
        key_size += sizeof(key->args.allgatherv);
        break;
    case SCATTERV:
        key_size += sizeof(key->args.scatterv);
        break;
    /* NON-BLOCKING COLLECTIVES */
    case IALLGATHER:
        key_size += sizeof(key->args.iallgather);
        break;
    case IALLGATHERV:
        key_size += sizeof(key->args.iallgatherv);
        break;
    case IALLREDUCE:
        key_size += sizeof(key->args.iallreduce);
        break;
    case IALLTOALL:
        key_size += sizeof(key->args.ialltoall);
        break;
    case IALLTOALLV:
        key_size += sizeof(key->args.ialltoallv);
        break;
    case IALLTOALLW:
        key_size += sizeof(key->args.ialltoallw);
        break;
    case IBARRIER:
        key_size += sizeof(key->args.ibarrier);
        break;
    case IBCAST:
        key_size += sizeof(key->args.ibcast);
        break;
    case IEXSCAN:
        key_size += sizeof(key->args.iexscan);
        break;
    case IGATHER:
        key_size += sizeof(key->args.igather);
        break;
    case IGATHERV:
        key_size += sizeof(key->args.igatherv);
        break;
    case IRED_SCAT:
        key_size += sizeof(key->args.ired_scat);
        break;
    case IRED_SCAT_BLOCK:
        key_size += sizeof(key->args.ired_scat_block);
        break;
    case IREDUCE:
        key_size += sizeof(key->args.ireduce);
        break;
    case ISCAN:
        key_size += sizeof(key->args.iscan);
        break;
    case ISCATTER:
        key_size += sizeof(key->args.iscatter);
        break;
    case ISCATTERV:
        key_size += sizeof(key->args.iscatterv);
    /* BLOCKING NEIGHBORHOOD COLLECTIVES */
        break;
    case NHB_ALLGATHER:
        key_size += sizeof(key->args.nhb_allgather);
        break;
    case NHB_ALLGATHERV:
        key_size += sizeof(key->args.nhb_allgatherv);
        break;
    case NHB_ALLTOALL:
        key_size += sizeof(key->args.nhb_alltoall);
        break;
    case NHB_ALLTOALLV:
        key_size += sizeof(key->args.nhb_alltoallv);
        break;
    case NHB_ALLTOALLW:
        key_size += sizeof(key->args.nhb_alltoallw);
    /* NON-BLOCKING NEIGHBORHOOD COLLECTIVES */
        break;
    case INHB_ALLGATHER:
        key_size += sizeof(key->args.inhb_allgather);
        break;
    case INHB_ALLGATHERV:
        key_size += sizeof(key->args.inhb_allgatherv);
        break;
    case INHB_ALLTOALL:
        key_size += sizeof(key->args.inhb_alltoall);
        break;
    case INHB_ALLTOALLV:
        key_size += sizeof(key->args.inhb_alltoallv);
        break;
    case INHB_ALLTOALLW:
        key_size += sizeof(key->args.inhb_alltoallw);
        break;

    default:
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "coll_type not recognized\n"));
        MPIR_Assert(0);
        break;
    }

    return key_size;
}
