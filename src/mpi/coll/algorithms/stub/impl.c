/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"

int MPIR_COLL_stub_init()
{
    return MPI_SUCCESS;
}

int MPIR_COLL_stub_comm_init(MPIR_Comm * comm)
{
    return MPI_SUCCESS;
}

int MPIR_COLL_stub_comm_init_null(MPIR_Comm * comm)
{
    return MPI_SUCCESS;
}

int MPIR_COLL_stub_comm_cleanup(MPIR_Comm * comm)
{
    return MPI_SUCCESS;
}
