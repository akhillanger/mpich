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

#include <stdio.h>
#include <string.h>
#include "queue.h"

#ifndef MPIR_TSP_NAMESPACE
#error "The collectives template must be namespaced with MPIR_TSP_NAMESPACE"
#endif

#include "coll_tree_schedule.h"

#undef FUNCNAME
#define FUNCNAME MPIR_TSP_tree_ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Non-blocking tree based broadcast */
int MPIR_TSP_tree_ibcast(void *buffer, int count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm, MPI_Request * req, int tree_type, int k, int segsize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_TREE_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_TREE_IBCAST);

    int mpi_errno = MPI_SUCCESS;
    *req = MPI_REQUEST_NULL;
    /* get the schedule */
    MPIR_TSP_sched_t *sched;
    mpi_errno =
        MPIR_TSP_get_bcast_tree_schedule(buffer, count, datatype, root, comm, tree_type, k, segsize,
                                         &sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    MPIR_Request *reqp = NULL;
    mpi_errno = MPIR_TSP_sched_start(sched, comm, &reqp);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    if (reqp)
        *req = reqp->handle;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_TREE_IBCAST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
