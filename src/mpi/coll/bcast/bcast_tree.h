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
#define FUNCNAME MPIR_TSP_tree_bcast_nb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Blocking broadcast */
int MPIR_TSP_tree_bcast_nb(void *buffer, int count, MPI_Datatype datatype, int root,
                        MPIR_Comm * comm, int *errflag, int tree_type, int k, int segsize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_TREE_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_TREE_BCAST);

    int mpi_errno = MPI_SUCCESS;
    MPI_Request req;
    mpi_errno =
        MPIR_TSP_tree_ibcast(buffer, count, datatype, root, comm, &req, tree_type, k, segsize);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Wait_impl(&req, MPI_STATUS_IGNORE);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_TREE_BCAST);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
