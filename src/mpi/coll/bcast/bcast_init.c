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

#include "mpiimpl.h"
#include "../ibcast/ibcast_tsp_tree_algos_prototypes.h"
#include "../ibcast/ibcast_tsp_scatter_recexch_allgather_algos_prototypes.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Bcast_init */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Bcast_init = PMPIX_Bcast_init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Bcast_init  MPIX_Bcast_init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Bcast_init as PMPIX_Bcast_init
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root,
                    MPI_Comm comm, MPI_Info info, MPI_Request * request)
    __attribute__ ((weak, alias("PMPIX_Bcast_init")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Bcast_init
#define MPIX_Bcast_init PMPIX_Bcast_init

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root,
                    MPIR_Comm * comm_ptr, MPIR_Info * info_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    /* create a new request */
    MPIR_Request *req = MPIR_Request_create(MPIR_REQUEST_KIND__PREQUEST_BCAST);
    if (!req)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* record bcast arguments */
    req->u.persist.coll_args.bcast.buffer = buffer;
    req->u.persist.coll_args.bcast.count = count;
    req->u.persist.coll_args.bcast.datatype = datatype;
    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    req->u.persist.coll_args.bcast.root = root;
    req->u.persist.coll_args.bcast.comm = comm_ptr;
    MPIR_Comm_add_ref(comm_ptr);
    req->u.persist.coll_args.bcast.info = info_ptr;
    req->u.persist.real_request = NULL;

    MPII_Genutil_sched_t *sched;
    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPII_Genutil_sched_t), MPL_MEM_COLL);
    MPII_Genutil_sched_create(sched, 1);
    
    switch(MPIR_Ibcast_intra_algo_choice) {
        case MPIR_IBCAST_INTRA_ALGO_GENTRAN_TREE:
            mpi_errno = MPII_Gentran_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                             MPIR_Ibcast_tree_type, MPIR_CVAR_IBCAST_TREE_KVAL,
                                                             MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE, sched);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
        case MPIR_IBCAST_INTRA_ALGO_GENTRAN_SCATTER_RECEXCH_ALLGATHER:
            mpi_errno = MPII_Gentran_Ibcast_sched_intra_scatter_recexch_allgather(buffer, count, datatype, root, comm_ptr, sched);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
        case MPIR_IBCAST_INTRA_ALGO_GENTRAN_RING:
            mpi_errno = MPII_Gentran_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr, MPIR_TREE_TYPE_KARY, 1, MPIR_CVAR_IBCAST_RING_CHUNK_SIZE, sched);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
        default:
            mpi_errno = MPII_Gentran_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm_ptr,
                                                             MPIR_Ibcast_tree_type, MPIR_CVAR_IBCAST_TREE_KVAL,
                                                             MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE, sched);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            break;
    }
    MPII_Genutil_sched_optimize(sched);
    req->u.persist.sched = sched;

    *request = req;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPIX_Bcast_init
#undef FCNAME
/*@
MPIX_Bcast_init - Creates a nonblocking, persistent collective communication request for the broadcast operation.

Input/Output Parameters:
. buffer - starting address of buffer (choice)

Input Parameters:
+ count - number of entries in buffer (integer)
. datatype - data type of buffer (handle)
. root - rank of broadcast root (integer)
. comm - communicator (handle)
. info - info argument (handle)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_ROOT

.seealso: MPI_Start, MPI_Startall, MPI_Request_free
@*/
int MPIX_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root,
                    MPI_Comm comm, MPI_Info info, MPI_Request * request)
{
    static const char FCNAME[] = "MPIX_Bcast_init";
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Info *info_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIX_BCAST_INIT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER(MPID_STATE_MPIX_BCAST_INIT);

    /* Validate handle parameters needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);
    MPIR_Info_get_ptr(info, info_ptr);

    /* Validate parameters if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            MPIR_Info_valid_ptr(info_ptr, mpi_errno);

            if (mpi_errno)
                goto fn_fail;

            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);

            /* Validate datatype handle */
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
            } else {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);
            }

            /* Validate datatype object */
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }

            MPIR_ERRTEST_BUF_INPLACE(buffer, count, mpi_errno);
            /* Validate buffer */
            MPIR_ERRTEST_USERBUFFER(buffer, count, datatype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Bcast_init(buffer, count, datatype, root, comm_ptr, info_ptr, &request_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* return the handle of the request to the user */
    MPIR_OBJ_PUBLISH_HANDLE(*request, request_ptr->handle);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPIX_BCAST_INIT);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpix_bcast_init", "**mpix_bcast_init %p %d %D %d %C %I %p",
                                 buffer, count, datatype, root, comm, info, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}