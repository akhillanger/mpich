/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "coll_util.h"
#include "ibcast.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ibcast */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ibcast = PMPI_Ibcast
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ibcast  MPI_Ibcast
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ibcast as PMPI_Ibcast
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm,
               MPI_Request *request) __attribute__((weak,alias("PMPI_Ibcast")));
#endif
/* -- End Profiling Symbol Block */

struct ibcast_status{
    int curr_bytes;
    int n_bytes;
    MPI_Status status;
};
/* Add some functions for asynchronous error detection */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ibcast
#define MPI_Ibcast PMPI_Ibcast

#undef FUNCNAME
#define FUNCNAME sched_test_length
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

static int sched_test_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    int recv_size;
    struct  ibcast_status *status = (struct ibcast_status*) state;
    MPIR_Get_count_impl(&status->status, MPI_BYTE, &recv_size);
    if(status->n_bytes != recv_size || status->status.MPI_ERROR != MPI_SUCCESS) {
         mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE,
               FCNAME, __LINE__, MPI_ERR_OTHER,
                     "**collective_size_mismatch",
                  "**collective_size_mismatch %d %d", status->n_bytes, recv_size);
    }
    return mpi_errno;
}

/* any non-MPI functions go here, especially non-static ones */

/* This routine purely handles the hierarchical version of bcast, and does not
 * currently make any decision about which particular algorithm to use for any
 * subcommunicator. */
static int ibcast_smp_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_homogeneous;
    MPI_Aint type_size;
    struct ibcast_status *status;
    MPIR_SCHED_CHKPMEM_DECL(1);

    if (!MPIR_CVAR_ENABLE_SMP_COLLECTIVES || !MPIR_CVAR_ENABLE_SMP_BCAST)
        MPID_Abort(comm_ptr, MPI_ERR_OTHER, 1, "SMP collectives are disabled!");
    MPIR_Assert(MPIR_Comm_is_node_aware(comm_ptr));
    MPIR_SCHED_CHKPMEM_MALLOC(status, struct ibcast_status*,
                              sizeof(struct ibcast_status), mpi_errno, "MPI_Status", MPL_MEM_BUFFER);

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    MPIR_Assert(is_homogeneous); /* we don't handle the hetero case yet */

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPIR_Datatype_get_size_macro(datatype, type_size);
    else
        MPIR_Pack_size_impl(1, datatype, &type_size);

     status->n_bytes = type_size * count;
    /* TODO insert packing here */

    /* send to intranode-rank 0 on the root's node */
    if (comm_ptr->node_comm != NULL &&
        MPIR_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */
    {                                                /* and is on our node (!-1) */
        if (root == comm_ptr->rank) {
            mpi_errno = MPIR_Sched_send(buffer, count, datatype, 0, comm_ptr->node_comm, s);
        }
        else if (0 == comm_ptr->node_comm->rank) {
            mpi_errno = MPIR_Sched_recv_status(buffer, count, datatype, MPIR_Get_intranode_rank(comm_ptr, root),
                                        comm_ptr->node_comm, &status->status, s);
        }
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
        MPIR_SCHED_BARRIER(s);
        mpi_errno = MPIR_Sched_cb(&sched_test_length, status, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* perform the internode broadcast */
    if (comm_ptr->node_roots_comm != NULL)
    {
        mpi_errno = MPID_Ibcast_sched(buffer, count, datatype,
                                      MPIR_Get_internode_rank(comm_ptr, root),
                                      comm_ptr->node_roots_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* don't allow the local ops for the intranode phase to start until this has completed */
        MPIR_SCHED_BARRIER(s);
    }
    /* perform the intranode broadcast on all except for the root's node */
    if (comm_ptr->node_comm != NULL)
    {
        mpi_errno = MPID_Ibcast_sched(buffer, count, datatype, 0, comm_ptr->node_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

/* Provides a generic "flat" broadcast that doesn't know anything about hierarchy.  It will choose
 * between several different algorithms based on the given parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_intra_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_intra_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, is_homogeneous ATTRIBUTE((unused));
    MPI_Aint type_size, nbytes;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    MPIR_Assert(is_homogeneous); /* we don't handle the hetero case right now */

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    /* simplistic implementation for now */
    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
        (comm_size < MPIR_CVAR_BCAST_MIN_PROCS))
    {
        mpi_errno = MPIR_Ibcast_binomial_sched(buffer, count, datatype, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else /* (nbytes >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_size >= MPIR_CVAR_BCAST_MIN_PROCS) */
    {
        if ((nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE) && (MPIU_is_pof2(comm_size, NULL))) {
            mpi_errno = MPII_Ibcast_scatter_rec_dbl_allgather_sched(buffer, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else {
            mpi_errno = MPII_Ibcast_scatter_ring_allgather_sched(buffer, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Provides a generic "flat" broadcast for intercommunicators that doesn't know
 * anything about hierarchy.  It will choose between several different
 * algorithms based on the given parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_inter_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_inter_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibcast_flat_sched(buffer, count, datatype, root, comm_ptr, s);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_sched(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT) {
            mpi_errno = ibcast_smp_sched(buffer, count, datatype, root, comm_ptr, s);
        } else {
            mpi_errno = MPIR_Ibcast_intra_sched(buffer, count, datatype, root, comm_ptr, s);
        }
    } else {
        mpi_errno = MPIR_Ibcast_inter_sched(buffer, count, datatype, root, comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *reqp = NULL;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPID_Ibcast_sched(buffer, count, datatype, root, comm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, &reqp);
    if (reqp)
        *request = reqp->handle;
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ibcast - Broadcasts a message from the process with rank "root" to
             all other processes of the communicator in a nonblocking way

Input/Output Parameters:
. buffer - starting address of buffer (choice)

Input Parameters:
+ count - number of entries in buffer (non-negative integer)
. datatype - datatype of buffer (handle)
. root - rank of broadcast root (integer)
- comm - communicator (handle)

Output Parameters:
. request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IBCAST);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IBCAST);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Ibcast(buffer, count, datatype, root, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IBCAST);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_ibcast", "**mpi_ibcast %p %d %D %C %p", buffer, count, datatype, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
