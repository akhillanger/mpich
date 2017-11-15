/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_PROGRESS_MAX_COLLS
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum number of collective operations at a time that the progress engine should make progress on

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include "coll_sched_db.h"
#include "generic_tsp_types.h"
#include "generic_tsp.h"
#include "generic_tsp_utils.h"

/*FIXME: MPIR_COLL_progress_global should be per-VNI */
MPIR_COLL_queue_t coll_queue;
static int progress_hook_id = 0;

UT_icd vtx_t_icd = { sizeof(MPIR_COLL_GENERIC_vtx_t), NULL, MPII_COLL_GENERIC_vtx_t_copy,
    MPII_COLL_GENERIC_vtx_t_dtor
};

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_init
/* Transport initialization function */
int MPIR_COLL_GENERIC_init()
{
    int mpi_errno = MPI_SUCCESS;
    TAILQ_INIT(&coll_queue.head);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_save_schedule
/* Transport function to store a schedule */
void MPIR_COLL_GENERIC_save_schedule(MPIR_Comm * comm_ptr, void *key, int len, void *s)
{
    MPIR_COLL_add_sched((MPIR_COLL_sched_entry_t **) & (comm_ptr->coll.sched_db), key, len, s);
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_init
/* Transport function to initialize a new schedule */
void MPIR_COLL_GENERIC_sched_init(MPIR_COLL_GENERIC_sched_t * sched, int tag)
{
    sched->total = 0;
    sched->num_completed = 0;
    sched->last_wait = -1;
    sched->tag = tag;

    /* allocate memory for storing vertices */
    utarray_new(sched->vtcs, &vtx_t_icd, MPL_MEM_COLL);
    /* initialize array for storing memory buffer addresses */
    utarray_new(sched->buf_array, &ut_ptr_icd, MPL_MEM_COLL);

    /* reset issued vertex list */
    MPII_COLL_GENERIC_reset_issued_list(sched);
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_reset
/* Transport function to reset an existing schedule */
void MPIR_COLL_GENERIC_sched_reset(MPIR_COLL_GENERIC_sched_t * sched, int tag)
{
    int i;

    sched->num_completed = 0;
    sched->tag = tag;   /* set new tag value */

    for (i = 0; i < sched->total; i++) {
        vtx_t *vtx = (vtx_t *) utarray_eltptr(sched->vtcs, i);
        vtx->state = MPIR_COLL_GENERIC_STATE_INIT;
        /* reset the number of unfinished dependencies to be the number of incoming vertices */
        vtx->num_unfinished_dependencies = utarray_len(vtx->invtcs);
        /* recv_reduce tasks need to be set as not done */
        if (vtx->kind == MPIR_COLL_GENERIC_KIND_RECV_REDUCE)
            vtx->nbargs.recv_reduce.done = 0;
        /* reset progress of multicast operation */
        if (vtx->kind == MPIR_COLL_GENERIC_KIND_MULTICAST)
            vtx->nbargs.multicast.last_complete = -1;
    }
    /* Reset issued vertex list to NULL.
     * **TODO: Check if this is really required as the vertex linked list
     * might be NULL implicitly at the end of the previous schedule use
     * */
    MPII_COLL_GENERIC_reset_issued_list(sched);
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_get_schedule
/* Transport function to get schedule (based on the key) if it exists, else return a new schedule */
MPIR_COLL_GENERIC_sched_t *MPIR_COLL_GENERIC_get_schedule(MPIR_Comm * comm_ptr,
                                                          void *key, int len, int tag, int *is_new)
{
    MPIR_COLL_GENERIC_sched_t *sched = MPIR_COLL_get_sched(comm_ptr->coll.sched_db, key, len);
    if (sched) {        /* schedule already exists */
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "Schedule is loaded from the database[%p]\n", sched));
        *is_new = 0;
        /* reset the schedule for reuse */
        MPIR_COLL_GENERIC_sched_reset(sched, tag);
    } else {
        *is_new = 1;
        sched = MPL_malloc(sizeof(MPIR_COLL_GENERIC_sched_t), MPL_MEM_COLL);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "New schedule is created:[%p]\n", sched));
        /* initialize the newly created schedule */
        MPIR_COLL_GENERIC_sched_init(sched, tag);
    }
    return sched;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_commit
/* Transport function to tell that the schedule generation is now complete */
void MPIR_COLL_GENERIC_sched_commit(MPIR_COLL_GENERIC_sched_t * sched)
{
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_finalize
/* Transport function to tell that the schedule can now be freed
 * and can also be removed from the database. We are not doing
 * anything here because the schedule is stored in the database
 * can be reused in future */
void MPIR_COLL_GENERIC_sched_finalize(MPIR_COLL_GENERIC_sched_t * sched)
{
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_fence
/* Transport function that adds a no op vertex in the graph that has
 * all the vertices posted before it as incoming vertices */
int MPIR_COLL_GENERIC_sched_fence(MPIR_COLL_GENERIC_sched_t * sched)
{
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [fence] total=%d \n", sched->total));

    vtx_t *vtxp;
    int *invtcs, i, n_invtcs = 0, vtx_id;

    vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);

    vtxp->kind = MPIR_COLL_GENERIC_KIND_FENCE;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);

    invtcs = (int *) MPL_malloc(sizeof(int) * vtx_id, MPL_MEM_COLL);
    /* record incoming vertices */
    for (i = vtx_id - 1; i >= 0; i--) {
        if (((vtx_t *) utarray_eltptr(sched->vtcs, i))->kind == MPIR_COLL_GENERIC_KIND_WAIT)
            /* no need to record this and any vertex before wait.
             * Dependency on the last wait call will be added by
             * the subsequent call to MPIR_COLL_GENERIC_add_vtx_dependencies function */
            break;
        else {
            invtcs[vtx_id - 1 - i] = i;
            n_invtcs++;
        }
    }

    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);
    MPL_free(invtcs);
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_wait_for
/* Transport function that adds a no op vertex in the graph with specified dependencies */
int MPIR_COLL_GENERIC_sched_wait_for(MPIR_COLL_GENERIC_sched_t * sched, int nvtcs, int *vtcs)
{
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [wait_for] total=%d \n", sched->total));
    vtx_t *vtxp;
    int vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);

    vtxp->kind = MPIR_COLL_GENERIC_KIND_NOOP;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);

    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, nvtcs, vtcs);
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_wait
/* MPIR_COLL_GENERIC_wait waits for all the operations posted before it to complete
* before issuing any operations posted after it. This is useful in composing
* multiple schedules, for example, allreduce can be written as
* MPIR_TSP_sched_reduce(s)
* MPIR_TSP_wait(s)
* MPIR_TSP_sched_bcast(s)
* This is different from the fence operation in the sense that fence requires
* a vertex to post dependencies on it while MPIR_COLL_GENERIC_wait is used internally
* by the transport to add it as a dependency to any operations posted after it
*/
int MPIR_COLL_GENERIC_sched_wait(MPIR_COLL_GENERIC_sched_t * sched)
{
    int wait_id;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Scheduling a MPIR_TSP_wait\n"));
    /* wait operation is an extension to fence, so we can reuse the fence call */
    wait_id = MPIR_COLL_GENERIC_sched_fence(sched);
    /* change the vertex kind from FENCE to WAIT */
    ((vtx_t *) utarray_eltptr(sched->vtcs, wait_id))->kind = MPIR_COLL_GENERIC_KIND_WAIT;
    sched->last_wait = wait_id;
    return wait_id;
}


#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_send
/* Transport function to schedule a send task */
int MPIR_COLL_GENERIC_sched_send(const void *buf,
                                 int count,
                                 MPI_Datatype dt,
                                 int dest,
                                 int tag,
                                 MPIR_Comm * comm_ptr,
                                 MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs)
{
    /* assign a new vertex */
    vtx_t *vtxp;
    int vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);
    sched->tag = tag;
    vtxp->kind = MPIR_COLL_GENERIC_KIND_SEND;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);
    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.sendrecv.buf = (void *) buf;
    vtxp->nbargs.sendrecv.count = count;
    vtxp->nbargs.sendrecv.dt = dt;
    vtxp->nbargs.sendrecv.dest = dest;
    vtxp->nbargs.sendrecv.comm = comm_ptr;
    /* second request is not used */
    vtxp->req[1] = NULL;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [%d] [send]\n", vtx_id));
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_multicast
int MPIR_COLL_GENERIC_sched_multicast(const void *buf,
                                      int count,
                                      MPI_Datatype dt,
                                      int *destinations,
                                      int num_destinations,
                                      int tag,
                                      MPIR_Comm * comm_ptr,
                                      MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs)
{
    /* assign a new vertex */
    vtx_t *vtxp;
    int vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);
    sched->tag = tag;
    vtxp->kind = MPIR_COLL_GENERIC_KIND_MULTICAST;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);
    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);
    /* store the arguments */
    vtxp->nbargs.multicast.buf = (void *) buf;
    vtxp->nbargs.multicast.count = count;
    vtxp->nbargs.multicast.dt = dt;
    vtxp->nbargs.multicast.num_destinations = num_destinations;
    vtxp->nbargs.multicast.destinations =
        (int *) MPL_malloc(sizeof(int) * num_destinations, MPL_MEM_COLL);
    memcpy(vtxp->nbargs.multicast.destinations, destinations, sizeof(int) * num_destinations);

    vtxp->nbargs.multicast.comm = comm_ptr;
    vtxp->nbargs.multicast.req =
        (struct MPIR_Request **) MPL_malloc(sizeof(struct MPIR_Request *) * num_destinations,
                                            MPL_MEM_COLL);
    vtxp->nbargs.multicast.last_complete = -1;
    /* set unused request pointers to NULL */
    vtxp->req[0] = NULL;
    vtxp->req[1] = NULL;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [%d] [send]\n", vtx_id));
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_send_accumulate
/* Transport function to schedule a send_accumulate task */
int MPIR_COLL_GENERIC_sched_send_accumulate(const void *buf,
                                            int count,
                                            MPI_Datatype dt,
                                            MPI_Op op,
                                            int dest,
                                            int tag,
                                            MPIR_Comm * comm_ptr,
                                            MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs,
                                            int *invtcs)
{
    /* assign a new vertex */
    vtx_t *vtxp;
    int vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);

    sched->tag = tag;
    vtxp->kind = MPIR_COLL_GENERIC_KIND_SEND;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);
    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.sendrecv.buf = (void *) buf;
    vtxp->nbargs.sendrecv.count = count;
    vtxp->nbargs.sendrecv.dt = dt;
    vtxp->nbargs.sendrecv.dest = dest;
    vtxp->nbargs.sendrecv.comm = comm_ptr;
    /* second request is not used */
    vtxp->req[1] = NULL;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [%d] [send_accumulate]\n", vtx_id));
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_recv
/* Transport function to schedule a recv task */
int MPIR_COLL_GENERIC_sched_recv(void *buf,
                                 int count,
                                 MPI_Datatype dt,
                                 int source,
                                 int tag,
                                 MPIR_Comm * comm_ptr,
                                 MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs)
{
    /* assign a new vertex */
    vtx_t *vtxp;
    int vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);

    sched->tag = tag;
    vtxp->kind = MPIR_COLL_GENERIC_KIND_RECV;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);
    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* record the arguments */
    vtxp->nbargs.sendrecv.buf = buf;
    vtxp->nbargs.sendrecv.count = count;
    vtxp->nbargs.sendrecv.dt = dt;
    vtxp->nbargs.sendrecv.dest = source;
    vtxp->nbargs.sendrecv.comm = comm_ptr;
    vtxp->req[1] = NULL;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [%d] [recv]\n", vtx_id));
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_recv_reduce
/* Transport function to schedule a recv_reduce call */
int MPIR_COLL_GENERIC_sched_recv_reduce(void *buf,
                                        int count,
                                        MPI_Datatype datatype,
                                        MPI_Op op,
                                        int source,
                                        int tag,
                                        MPIR_Comm * comm_ptr,
                                        uint64_t flags,
                                        MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs,
                                        int *invtcs)
{
    size_t extent;
    MPI_Aint true_extent, lower_bound;
    vtx_t *vtxp;

    /* assign a vertex */
    int vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);
    sched->tag = tag;
    vtxp->kind = MPIR_COLL_GENERIC_KIND_RECV_REDUCE;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);
    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lower_bound, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    vtxp->nbargs.recv_reduce.inbuf = MPL_malloc(count * extent, MPL_MEM_COLL);
    vtxp->nbargs.recv_reduce.inoutbuf = buf;
    vtxp->nbargs.recv_reduce.count = count;
    vtxp->nbargs.recv_reduce.datatype = datatype;
    vtxp->nbargs.recv_reduce.op = op;
    vtxp->nbargs.recv_reduce.source = source;
    vtxp->nbargs.recv_reduce.comm = comm_ptr;
    vtxp->nbargs.recv_reduce.vtxp = vtxp;
    vtxp->nbargs.recv_reduce.done = 0;
    vtxp->nbargs.recv_reduce.flags = flags;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [%d] [recv_reduce]\n", vtx_id));

    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_reduce_local
/* Transport function to schedule a local reduce */
int MPIR_COLL_GENERIC_sched_reduce_local(const void *inbuf,
                                         void *inoutbuf,
                                         int count,
                                         MPI_Datatype datatype,
                                         MPI_Op operation,
                                         uint64_t flags,
                                         MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs,
                                         int *invtcs)
{
    vtx_t *vtxp;
    int vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);

    /* assign a new vertex */
    vtxp->kind = MPIR_COLL_GENERIC_KIND_REDUCE_LOCAL;
    vtxp->state = MPIR_COLL_GENERIC_STATE_INIT;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);
    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.reduce_local.inbuf = inbuf;
    vtxp->nbargs.reduce_local.inoutbuf = inoutbuf;
    vtxp->nbargs.reduce_local.count = count;
    vtxp->nbargs.reduce_local.dt = datatype;
    vtxp->nbargs.reduce_local.op = operation;
    vtxp->nbargs.reduce_local.flags = flags;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [%d] [reduce_local]\n", vtx_id));
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_dt_copy
/* Transport function to schedule a data copy */
int MPIR_COLL_GENERIC_sched_dt_copy(void *tobuf,
                                    int tocount,
                                    MPI_Datatype totype,
                                    const void *frombuf,
                                    int fromcount,
                                    MPI_Datatype fromtype,
                                    MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs)
{
    vtx_t *vtxp;
    int vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);

    /* assign a new vertex */
    vtxp->kind = MPIR_COLL_GENERIC_KIND_DT_COPY;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);
    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.dt_copy.tobuf = tobuf;
    vtxp->nbargs.dt_copy.tocount = tocount;
    vtxp->nbargs.dt_copy.totype = totype;
    vtxp->nbargs.dt_copy.frombuf = frombuf;
    vtxp->nbargs.dt_copy.fromcount = fromcount;
    vtxp->nbargs.dt_copy.fromtype = fromtype;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [%d] [dt_copy]\n", vtx_id));
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_allocate_buffer
/* Transport function to allocate memory required for schedule execution.
 *The memory address is recorded by the transport in the schedule
 * so that this memory can be freed when the schedule is destroyed
 */
void *MPIR_COLL_GENERIC_allocate_buffer(size_t size, MPIR_COLL_GENERIC_sched_t * s)
{
    void *buf = MPL_malloc(size, MPL_MEM_COLL);
    /* record memory allocation */
    utarray_push_back(s->buf_array, &buf, MPL_MEM_COLL);
    return buf;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_free_mem
/* Transport function to schedule memory free'ing */
int MPIR_COLL_GENERIC_sched_free_mem(void *ptr,
                                     MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs)
{
    vtx_t *vtxp;
    int vtx_id = MPII_COLL_GENERIC_get_new_vtx(sched, &vtxp);

    /* assign a new vertex */
    vtxp->kind = MPIR_COLL_GENERIC_KIND_FREE_MEM;
    MPII_COLL_GENERIC_init_vtx(sched, vtxp, vtx_id);
    MPII_COLL_GENERIC_add_vtx_dependencies(sched, vtx_id, n_invtcs, invtcs);

    /* store the arguments */
    vtxp->nbargs.free_mem.ptr = ptr;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "TSP(generic) : sched [%d] [free_mem]\n", vtx_id));
    return vtx_id;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_sched_poke
/* Transport function to make progress on the schedule */
int MPIR_COLL_GENERIC_sched_poke(MPIR_COLL_GENERIC_sched_t * sched, bool * is_complete)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COLL_GENERIC_SCHED_POKE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COLL_GENERIC_SCHED_POKE);

    int mpi_errno = MPI_SUCCESS;

    vtx_t *vtxp;
    int i;
    /* If issued list is empty, go over the vertices and issue ready vertices
     * Issued list should be empty only when the MPIR_COLL_GENERIC_poke function
     * is called for the first time on the schedule */
    if (sched->issued_head == NULL) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "issued list is empty, issue ready vtcs\n"));
        if (sched->total > 0 && sched->num_completed != sched->total) {
            /* Go over all the vertices and issue ready vertices */
            for (i = 0; i < sched->total; i++) {
                MPII_COLL_GENERIC_issue_vtx(i, (vtx_t *) utarray_eltptr(sched->vtcs, i), sched);
            }

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "completed traversal of vtcs, sched->total: %d, sched->num_completed: %d\n",
                             sched->total, sched->num_completed));
            *is_complete = 0;
            goto fn_exit;
        } else {
            *is_complete = 1;   /* schedule completed */
            goto fn_exit;
        }
    }

    if (sched->total == sched->num_completed) { /* if done, return */
        *is_complete = 1;
        goto fn_exit;
    }

    /* iterate over the issued vertex list, starting at the head */
    sched->vtx_iter = sched->issued_head;
    /* reset the issued vertex list, the list will be created again */
    sched->issued_head = NULL;

    /* Check for issued vertices that have been completed */
    while (sched->vtx_iter != NULL) {
        vtxp = sched->vtx_iter;
        /* vtx_iter is updated to point to the next issued vertex
         * that will be looked at after the current vertex */
        sched->vtx_iter = sched->vtx_iter->next_issued;

        if (vtxp->state == MPIR_COLL_GENERIC_STATE_ISSUED) {

            MPI_Status status;
            MPIR_Request *req0 = vtxp->req[0];
            MPIR_Request *req1 = vtxp->req[1];

            /* If recv_reduce (req is not NULL), then make progress on it */
            if (req1) {
                (req1->u.ureq.greq_fns->query_fn)
                    (req1->u.ureq.greq_fns->grequest_extra_state, &status);
            }

            switch (vtxp->kind) {
            case MPIR_COLL_GENERIC_KIND_SEND:
            case MPIR_COLL_GENERIC_KIND_RECV:
                if (req0 && MPIR_Request_is_complete(req0)) {
                    MPIR_Request_free(req0);
                    vtxp->req[0] = NULL;
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "  --> GENERIC transport (kind=%d) complete\n",
                                     vtxp->kind));
                    if (vtxp->nbargs.sendrecv.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST, "data sent/recvd: %d\n",
                                         *(int *) (vtxp->nbargs.sendrecv.buf)));
#endif
                    /* record vertex completion */
                    MPII_COLL_GENERIC_record_vtx_completion(vtxp, sched);
                } else
                    MPII_COLL_GENERIC_record_vtx_issue(vtxp, sched);    /* record it again as issued */
                break;
            case MPIR_COLL_GENERIC_KIND_MULTICAST:
                for (i = vtxp->nbargs.multicast.last_complete + 1;
                     i < vtxp->nbargs.multicast.num_destinations; i++) {
                    if (MPIR_Request_is_complete(vtxp->nbargs.multicast.req[i])) {
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST,
                                         "  --> GENERIC transport multicast task %d complete\n",
                                         i));
                        MPIR_Request_free(vtxp->nbargs.multicast.req[i]);
                        vtxp->nbargs.multicast.req[i] = NULL;
                        vtxp->nbargs.multicast.last_complete = i;
                    } else      /* we are only checking in sequence, hence break out at the first incomplete send */
                        break;
                }
                /* if all the sends have completed, mark this vertex as complete */
                if (i == vtxp->nbargs.multicast.num_destinations)
                    MPII_COLL_GENERIC_record_vtx_completion(vtxp, sched);
                else
                    MPII_COLL_GENERIC_record_vtx_issue(vtxp, sched);
                break;
            case MPIR_COLL_GENERIC_KIND_RECV_REDUCE:
                /* check for recv completion */
                if (req0 && MPIR_Request_is_complete(req0)) {
                    MPIR_Request_free(req0);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "recv in recv_reduce completed\n"));
                    vtxp->req[0] = NULL;
                }

                /* check for reduce completion */
                if (req1 && MPIR_Request_is_complete(req1)) {
                    MPIR_Request_free(req1);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "reduce in recv_reduce also completed\n"));
                    vtxp->req[1] = NULL;

                    /* recv_reduce is now complete because reduce can complete
                     * only if the corresponding recv is complete */
#ifdef MPL_USE_DBG_LOGGING
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "  --> GENERIC transport (kind=%d) complete\n",
                                     vtxp->kind));
                    if (vtxp->nbargs.sendrecv.count >= 1)
                        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                        (MPL_DBG_FDEST, "data sent/recvd: %d\n",
                                         *(int *) (vtxp->nbargs.sendrecv.buf)));
#endif
                    /* record vertex completion */
                    MPII_COLL_GENERIC_record_vtx_completion(vtxp, sched);
                } else {
                    MPII_COLL_GENERIC_record_vtx_issue(vtxp, sched);    /* record it again as issued */
                }
                break;
            default:
                break;
            }
        }
    }
    /* set the tail of the issued vertex linked lsit */
    sched->last_issued->next_issued = NULL;

#ifdef MPL_USE_DBG_LOGGING
    if (sched->num_completed == sched->total) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "  --> GENERIC transport (test) complete:  sched->total=%d\n",
                         sched->total));
    }
#endif

    /* set is_complete */
    *is_complete = (sched->num_completed == sched->total);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COLL_GENERIC_SCHED_POKE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_comm_init
/* Transport function to initialize a transport communicator */
int MPIR_COLL_GENERIC_comm_init(MPIR_Comm * comm_ptr)
{
    comm_ptr->coll.sched_db = NULL;
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_comm_init_null
/* Transport function to initialize a transport communicator data to NULL */
int MPIR_COLL_GENERIC_comm_init_null(MPIR_Comm * comm_ptr)
{
    comm_ptr->coll.sched_db = NULL;
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_comm_cleanup
/* Transport function to cleanup a transport communicator */
int MPIR_COLL_GENERIC_comm_cleanup(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "In MPIR_COLL_GENERIC_comm_cleanup for comm: %p\n", comm_ptr));
    /* free up schedule database */
    if (comm_ptr->coll.sched_db)
        MPIR_COLL_delete_sched_table(comm_ptr->coll.sched_db,
                                     (MPIR_COLL_sched_free_fn) MPII_COLL_GENERIC_sched_destroy_fn);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_wait_sched
/* Transport function to wait for a schedule to complete execution, used for blocking
 * collectives */
int MPIR_COLL_GENERIC_wait_sched(MPIR_COLL_GENERIC_sched_t * sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COLL_GENERIC_WAIT_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COLL_GENERIC_WAIT_SCHED);

    int mpi_errno = MPI_SUCCESS;
    bool is_complete = false;
    while (!is_complete) {
        mpi_errno = MPIR_COLL_GENERIC_sched_poke(sched, &is_complete);  /* Make progress on the collective schedule */
        if (!is_complete) {
            MPID_Progress_test();
        }
    }

    /* Mark sched as completed */
    MPIR_COLL_GENERIC_sched_finalize(sched);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COLL_GENERIC_WAIT_SCHED);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_progress_nbc
/* Function to make progress on a non-blocking collective */
int MPIR_COLL_GENERIC_progress_nbc(MPIR_COLL_req_t * coll_req, bool * done)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COLL_GENERIC_PROGRESS_NBC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COLL_GENERIC_PROGRESS_NBC);

    int mpi_errno = MPI_SUCCESS;
    MPIR_COLL_GENERIC_sched_t *sched = (MPIR_COLL_GENERIC_sched_t *) (coll_req->sched);

    /* make progress on the collective */
    mpi_errno = MPIR_COLL_GENERIC_sched_poke(sched, done);

    if (*done) {
        /* delete all memory associated with the schedule */
        MPIR_COLL_GENERIC_sched_finalize(sched);
        coll_req->sched = NULL;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COLL_GENERIC_PROGRESS_NBC);

    return mpi_errno;
}

/* Function that tells if there are pending schedules */
bool MPIR_COLL_GENERIC_sched_are_pending() {
    return coll_queue.head.tqh_first != NULL;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_GENERIC_enqueue_and_poke_nbc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* Transport function to enqueue and kick start a non-blocking collective */
int MPIR_COLL_GENERIC_sched_start(MPIR_COLL_GENERIC_sched_t * sched, MPIR_Comm * comm,
                                  MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COLL_GENERIC_SCHED_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COLL_GENERIC_SCHED_START);

    /* now create and populate the request */
    MPIR_Request *r = MPIR_Request_create(MPIR_REQUEST_KIND__COLL);
    if (!r)
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    *req = r;
    MPIR_Request_add_ref(r);

    /* Make some progress now */
    bool is_complete;
    mpi_errno = MPIR_COLL_GENERIC_sched_poke(sched, &is_complete);

    if(is_complete) {
        MPID_Request_complete(r);
        goto fn_exit;
    }
    /* Enqueue nonblocking collective  */

    r->coll.sched = (void *) sched;

    if (coll_queue.head.tqh_first == NULL) {
        mpi_errno = MPID_Progress_register_hook(MPIR_COLL_GENERIC_progress_hook, &progress_hook_id);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        MPID_Progress_activate_hook(progress_hook_id);
    }
    /* FIXME: MPIR_COLL_progress_global should be per-VNI  */
    TAILQ_INSERT_TAIL(&coll_queue.head, &r->coll, list_data);


    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COLL_GENERIC_SCHED_START);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Hook to make progress on non-blocking collective operations  */
int MPIR_COLL_GENERIC_progress_hook(int *made_progress)
{
    int count = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIR_COLL_req_t *coll_req, *coll_req_tmp;
    if (made_progress)
        *made_progress = false;

    /* Go over up to MPIR_COLL_PROGRESS_MAX_COLLS collecives in the queue and make progress on them  */
    TAILQ_FOREACH_SAFE(coll_req, &coll_queue.head, list_data, coll_req_tmp) {
        /* makeprogress on the collective operation  */
        bool done;
        mpi_errno = MPIR_COLL_GENERIC_progress_nbc(coll_req, &done);

        if (done) {
            /* If the collective operation has completed, mark it as complete  */
            MPIR_Request *req = MPL_container_of(coll_req, MPIR_Request, coll);
            TAILQ_REMOVE(&coll_queue.head, coll_req, list_data);
            MPID_Request_complete(req);
            if (made_progress)
                *made_progress = true;
            count++;
        }
        if (count >= MPIR_CVAR_PROGRESS_MAX_COLLS)
            break;
    }

    if (coll_queue.head.tqh_first == NULL) {
        MPID_Progress_deactivate_hook(progress_hook_id);
        MPID_Progress_deregister_hook(progress_hook_id);
    }

    return mpi_errno;
}
