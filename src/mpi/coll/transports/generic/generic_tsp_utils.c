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
#include "coll_sched_db.h"
#include "generic_tsp_types.h"
#include "generic_tsp.h"
#include "generic_tsp_utils.h"

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_vtx_t_copy
void MPII_COLL_GENERIC_vtx_t_copy(void *_dst, const void *_src)
{
    vtx_t *dst = (vtx_t *) _dst;
    vtx_t *src = (vtx_t *) _src;

    dst->kind = src->kind;
    dst->state = src->state;
    dst->req[0] = src->req[0];
    dst->req[1] = src->req[1];
    dst->id = src->id;

    utarray_new(dst->invtcs, &ut_int_icd, MPL_MEM_COLL);
    utarray_concat(dst->invtcs, src->invtcs, MPL_MEM_COLL);
    utarray_new(dst->outvtcs, &ut_int_icd, MPL_MEM_COLL);
    utarray_concat(dst->outvtcs, src->outvtcs, MPL_MEM_COLL);

    dst->num_unfinished_dependencies = src->num_unfinished_dependencies;
    dst->nbargs = src->nbargs;
    dst->next_issued = src->next_issued;

    if (dst->kind == MPIR_COLL_GENERIC_KIND_RECV_REDUCE) {
        dst->nbargs.recv_reduce.vtxp = dst;
    }
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_vtx_t_dtor
void MPII_COLL_GENERIC_vtx_t_dtor(void *_elt)
{
    vtx_t *elt = (vtx_t *) _elt;

    utarray_free(elt->invtcs);
    utarray_free(elt->outvtcs);
}


#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_decrement_num_unfinished_dependecies
/* Internal transport function. This function is called whenever a vertex completes its execution.
 * It notifies the outgoing vertices of the completed vertex about its completion by decrementing
 * the number of their number of unfinished dependencies */
void MPII_COLL_GENERIC_decrement_num_unfinished_dependecies(MPIR_COLL_GENERIC_vtx_t * vtxp,
                                                            MPIR_COLL_GENERIC_sched_t * sched)
{
    int i;

    /* Get the list of outgoing vertices */
    UT_array *outvtcs = vtxp->outvtcs;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Number of outgoing vertices of %d = %d\n", vtxp->id,
                     utarray_len(outvtcs)));
    /* for each outgoing vertex of vertex *vtxp, decrement number of unfinished dependencies */
    for (i = 0; i < utarray_len(outvtcs); i++) {
        int outvtx_id = *(int *) utarray_eltptr(outvtcs, i);
        int num_unfinished_dependencies =
            --(((vtx_t *) utarray_eltptr(sched->vtcs, outvtx_id))->num_unfinished_dependencies);
        /* if all dependencies of the outgoing vertex are complete, issue the vertex */
        if (num_unfinished_dependencies == 0) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "Issuing vertex number %d\n", outvtx_id));
            MPII_COLL_GENERIC_issue_vtx(outvtx_id, (vtx_t *) utarray_eltptr(sched->vtcs, outvtx_id),
                                        sched);
        }
    }
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_record_vtx_completion
/* Internal transport function to record completion of a vertex and take appropriate actions */
void MPII_COLL_GENERIC_record_vtx_completion(MPIR_COLL_GENERIC_vtx_t * vtxp,
                                             MPIR_COLL_GENERIC_sched_t * sched)
{
    vtxp->state = MPIR_COLL_GENERIC_STATE_COMPLETE;
    sched->num_completed++;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Number of completed vertices = %d\n", sched->num_completed));

    /* update outgoing vertices about this vertex completion */
    MPII_COLL_GENERIC_decrement_num_unfinished_dependecies(vtxp, sched);
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_record_vtx_issue
/* Internal transport function to record issue of a veretex */
void MPII_COLL_GENERIC_record_vtx_issue(MPIR_COLL_GENERIC_vtx_t * vtxp,
                                        MPIR_COLL_GENERIC_sched_t * sched)
{
    vtxp->state = MPIR_COLL_GENERIC_STATE_ISSUED;

    /* Update the linked list of issued vertices */

    /* If this is the first issued vertex in the linked list */
    if (sched->issued_head == NULL) {
        sched->issued_head = sched->last_issued = vtxp;
        sched->last_issued->next_issued = sched->vtx_iter;
    }
    /* If this is an already issued vertex that is part of the old linked list */
    else if (sched->last_issued->next_issued == vtxp) {
        sched->last_issued = vtxp;
    }
    /* Else, this is a newly issued vertex - insert it between last_issued and vtx_iter */
    else {
        sched->last_issued->next_issued = vtxp;
        vtxp->next_issued = sched->vtx_iter;
        sched->last_issued = vtxp;
    }

#ifdef MPL_USE_DBG_LOGGING
    /* print issued vertex list */
    MPIR_COLL_GENERIC_vtx_t *vtx = sched->issued_head;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Issued vertices list: "));
    while (vtx) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "%d ", vtx->id));
        vtx = vtx->next_issued;
    }
    /* print the current location of vertex iterator */
    if (sched->vtx_iter) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, ", vtx_iter: %d\n", sched->vtx_iter->id));
    } else {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "\n"));
    }
#endif

}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_reset_issued_list
/* Internal transport function to reset issued vertex list */
void MPII_COLL_GENERIC_reset_issued_list(MPIR_COLL_GENERIC_sched_t * sched)
{
    int i;

    sched->issued_head = NULL;
    /* If the schedule is being reused, reset only used vtcs because
     * it is known that only that many vertices will ever be used.
     * Else it is a new schedule being generated and therefore
     * reset all the vtcs* */
    for (i = 0; i < sched->total; i++) {
        ((vtx_t *) utarray_eltptr(sched->vtcs, i))->next_issued = NULL;
    }

    sched->vtx_iter = NULL;
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_allocate_vtx
/* Internal transport function to allocate memory associated with a graph vertex */
void MPII_COLL_GENERIC_allocate_vtx(MPIR_COLL_GENERIC_vtx_t * vtx)
{
    /* allocate memory for storing incoming and outgoing vertices */
    utarray_new(vtx->invtcs, &ut_int_icd, MPL_MEM_COLL);
    utarray_new(vtx->outvtcs, &ut_int_icd, MPL_MEM_COLL);
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_init_vtx
/* Internal transport function to initialize a graph vertex */
void MPII_COLL_GENERIC_init_vtx(MPIR_COLL_GENERIC_sched_t * sched, MPIR_COLL_GENERIC_vtx_t * vtx,
                                int id)
{
    /* allocate memory for storing incoming and outgoing edges */
    MPII_COLL_GENERIC_allocate_vtx(vtx);

    vtx->state = MPIR_COLL_GENERIC_STATE_INIT;
    vtx->id = id;
    vtx->num_unfinished_dependencies = 0;
    vtx->next_issued = NULL;
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_free_vtx
/* Internal transport function to free the memory associated with a vertex */
void MPII_COLL_GENERIC_free_vtx(MPIR_COLL_GENERIC_vtx_t * vtx)
{
    utarray_free(vtx->invtcs);
    utarray_free(vtx->outvtcs);
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_extend_int_utarray
void MPII_COLL_GENERIC_extend_int_utarray(UT_array * dst_array, int n_elems, int *elems)
{
    int i;
    for (i = 0; i < n_elems; i++) {
        utarray_push_back(dst_array, &elems[i], MPL_MEM_COLL);
    }
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_add_vtx_dependencies
/* Internal transport function to add incoming vertices of a vertex.
 * This vertex sets the incoming vertices to vtx and also adds vtx to the
 * outgoing vertex list of the vertives in invtcs.
 * NOTE: This function should only be called when a new vertex is added to the groph */
void MPII_COLL_GENERIC_add_vtx_dependencies(MPIR_COLL_GENERIC_sched_t * sched, int vtx_id,
                                            int n_invtcs, int *invtcs)
{
    int i;
    UT_array *outvtcs;
    MPIR_COLL_GENERIC_vtx_t *vtx = (vtx_t *) utarray_eltptr(sched->vtcs, vtx_id);
    UT_array *in = vtx->invtcs;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "Updating invtcs of vtx %d, kind %d, in->used %d, n_invtcs %d\n", vtx_id,
                     vtx->kind, utarray_len(in), n_invtcs));
    /* insert the incoming edges */
    MPII_COLL_GENERIC_extend_int_utarray(in, n_invtcs, invtcs);

    /* update the list of outgoing vertices of the incoming vertices */
    for (i = 0; i < n_invtcs; i++) {
        int in_vtx_id = *(int *) utarray_eltptr(in, i);
        vtx_t *in_vtx = (vtx_t *) utarray_eltptr(sched->vtcs, in_vtx_id);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "invtx: %d\n", in_vtx_id));
        outvtcs = in_vtx->outvtcs;
        MPII_COLL_GENERIC_extend_int_utarray(outvtcs, 1, &vtx_id);

        /* increment num_unfinished_dependencies only is the incoming vertex is not complete yet */
        if (in_vtx->state != MPIR_COLL_GENERIC_STATE_COMPLETE)
            vtx->num_unfinished_dependencies++;
    }

    /* check if there was any MPIR_COLL_GENERIC_wait operation and add appropriate dependencies.
     * MPIR_TSP_wait function does not return a vertex id, so the application will never explicity
     * specify a dependency on it, the transport has to make sure that the dependency
     * on the wait operation is met */
    if (sched->last_wait != -1 && sched->last_wait != vtx_id) {
        /* add the last wait vertex as an incoming vertex to vtx */
        MPII_COLL_GENERIC_extend_int_utarray(in, 1, &(sched->last_wait));

        /* add vtx as outgoing vtx of last_wait */
        vtx_t *last_wait_vtx = (vtx_t *) utarray_eltptr(sched->vtcs, sched->last_wait);
        outvtcs = last_wait_vtx->outvtcs;
        MPII_COLL_GENERIC_extend_int_utarray(outvtcs, 1, &vtx_id);

        if (last_wait_vtx->state != MPIR_COLL_GENERIC_STATE_COMPLETE)
            vtx->num_unfinished_dependencies++;
    }
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_get_new_vtx
/* Internal transport function to get a new vertex in the graph */
int MPII_COLL_GENERIC_get_new_vtx(MPIR_COLL_GENERIC_sched_t * sched, MPIR_COLL_GENERIC_vtx_t ** vtx)
{
    utarray_extend_back(sched->vtcs, MPL_MEM_COLL);
    *vtx = (vtx_t *) utarray_back(sched->vtcs);
    return sched->total++;      /* return vertex id */
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_do_reduce_in_recv_reduce
/* Internal transport function to do the reduce in the recv_reduce call */
int MPII_COLL_GENERIC_do_reduce_in_recv_reduce(void *data, MPI_Status * status)
{
    MPIR_COLL_GENERIC_recv_reduce_arg_t *rr = (MPIR_COLL_GENERIC_recv_reduce_arg_t *) data;
    /* Check if recv has completed (req[0]==NULL) and reduce has not already been done (!rr->done) */
    if (rr->vtxp->req[0] == NULL && !rr->done) {
        MPI_Datatype dt = rr->datatype;
        MPI_Op op = rr->op;

        if (rr->flags == -1 || rr->flags & MPIR_COLL_FLAG_REDUCE_L) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "  --> GENERIC transport (recv_reduce L) complete to %p\n",
                             rr->inoutbuf));

            MPIR_Reduce_local(rr->inbuf, rr->inoutbuf, rr->count, dt, op);
        } else {
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "  --> GENERIC transport (recv_reduce R) complete to %p\n",
                             rr->inoutbuf));

            MPIR_Reduce_local(rr->inoutbuf, rr->inbuf, rr->count, dt, op);
            MPIR_Localcopy(rr->inbuf, rr->count, dt, rr->inoutbuf, rr->count, dt);
        }

        MPIR_Grequest_complete_impl(rr->vtxp->req[1]);
        rr->done = 1;   /* recv_reduce is complete now */
    }

    /* TODO: Check if this has to be done everytime */
    status->MPI_SOURCE = MPI_UNDEFINED;
    status->MPI_TAG = MPI_UNDEFINED;
    MPIR_STATUS_SET_CANCEL_BIT(*status, FALSE);
    MPIR_Status_set_elements_x_impl(status, MPI_BYTE, 0);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_issue_vtx
/* Internal transport function to issue a vertex */
/* This is set to be static inline because compiler cannot build this function with MPL_STATIC_INLINE_PREFIX
 * complaining that this is a non-inlineable function */
void MPII_COLL_GENERIC_issue_vtx(int vtxid, MPIR_COLL_GENERIC_vtx_t * vtxp,
                                 MPIR_COLL_GENERIC_sched_t * sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COLL_GENERIC_ISSUE_VTX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COLL_GENERIC_ISSUE_VTX);

    int i;
    /* Check if the vertex has not already been issued and its incoming dependencies have completed */
    if (vtxp->state == MPIR_COLL_GENERIC_STATE_INIT && vtxp->num_unfinished_dependencies == 0) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Issuing vertex %d\n", vtxid));

        switch (vtxp->kind) {
        case MPIR_COLL_GENERIC_KIND_SEND:{
                MPIR_Errflag_t errflag = MPIR_ERR_NONE;
                /* issue task */
                MPIC_Isend(vtxp->nbargs.sendrecv.buf,
                           vtxp->nbargs.sendrecv.count,
                           vtxp->nbargs.sendrecv.dt,
                           vtxp->nbargs.sendrecv.dest,
                           sched->tag, vtxp->nbargs.sendrecv.comm, &vtxp->req[0], &errflag);
                /* record vertex issue */
                MPII_COLL_GENERIC_record_vtx_issue(vtxp, sched);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "  --> GENERIC transport (isend) issued, tag = %d\n", sched->tag));
            }
            break;

        case MPIR_COLL_GENERIC_KIND_RECV:{
                /* issue task */
                MPIC_Irecv(vtxp->nbargs.sendrecv.buf,
                           vtxp->nbargs.sendrecv.count,
                           vtxp->nbargs.sendrecv.dt,
                           vtxp->nbargs.sendrecv.dest,
                           sched->tag, vtxp->nbargs.sendrecv.comm, &vtxp->req[0]);
                /* record vertex issue */
                MPII_COLL_GENERIC_record_vtx_issue(vtxp, sched);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "  --> GENERIC transport (irecv) issued\n"));
            }
            break;
        case MPIR_COLL_GENERIC_KIND_MULTICAST:{
                MPIR_Errflag_t errflag = MPIR_ERR_NONE;
                /* issue task */
                for (i = 0; i < vtxp->nbargs.multicast.num_destinations; i++)
                    MPIC_Isend(vtxp->nbargs.multicast.buf,
                               vtxp->nbargs.multicast.count,
                               vtxp->nbargs.multicast.dt,
                               vtxp->nbargs.multicast.destinations[i],
                               sched->tag,
                               vtxp->nbargs.multicast.comm, &vtxp->nbargs.multicast.req[i],
                               &errflag);
                /* record vertex issue */
                MPII_COLL_GENERIC_record_vtx_issue(vtxp, sched);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "  --> GENERIC transport (multicast) issued, tag = %d\n",
                                 sched->tag));
            }
            break;

        case MPIR_COLL_GENERIC_KIND_DT_COPY:
            MPIR_Localcopy(vtxp->nbargs.dt_copy.frombuf,
                           vtxp->nbargs.dt_copy.fromcount,
                           vtxp->nbargs.dt_copy.fromtype,
                           vtxp->nbargs.dt_copy.tobuf,
                           vtxp->nbargs.dt_copy.tocount, vtxp->nbargs.dt_copy.totype);

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "  --> GENERIC transport (dt_copy) complete\n"));
            /* record vertex completion */
            MPII_COLL_GENERIC_record_vtx_completion(vtxp, sched);

            break;

        case MPIR_COLL_GENERIC_KIND_FREE_MEM:
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "  --> GENERIC transport (freemem) complete\n"));
            /* We are not really free'ing memory here, it will be needed when schedule is reused
             * The memory is free'd when the schedule is destroyed */

            /* record vertex completion */
            MPII_COLL_GENERIC_record_vtx_completion(vtxp, sched);
            break;

        case MPIR_COLL_GENERIC_KIND_NOOP:
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "  --> GENERIC transport (noop) complete\n"));
            MPII_COLL_GENERIC_record_vtx_completion(vtxp, sched);
            break;

        case MPIR_COLL_GENERIC_KIND_FENCE:
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "  --> GENERIC transport (fence) complete\n"));
            MPII_COLL_GENERIC_record_vtx_completion(vtxp, sched);
            break;

        case MPIR_COLL_GENERIC_KIND_WAIT:
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "  --> GENERIC transport (wait) complete\n"));
            MPII_COLL_GENERIC_record_vtx_completion(vtxp, sched);
            break;

        case MPIR_COLL_GENERIC_KIND_RECV_REDUCE:{
                MPIC_Irecv(vtxp->nbargs.recv_reduce.inbuf,
                           vtxp->nbargs.recv_reduce.count,
                           vtxp->nbargs.recv_reduce.datatype,
                           vtxp->nbargs.recv_reduce.source,
                           sched->tag, vtxp->nbargs.recv_reduce.comm, &vtxp->req[0]);
                MPIR_Grequest_start_impl(MPII_COLL_GENERIC_do_reduce_in_recv_reduce, NULL, NULL,
                                         &vtxp->nbargs.recv_reduce, &vtxp->req[1]);

                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "  --> GENERIC transport (recv_reduce) issued\n"));

                /* record vertex issue */
                MPII_COLL_GENERIC_record_vtx_issue(vtxp, sched);
            }
            break;

        case MPIR_COLL_GENERIC_KIND_REDUCE_LOCAL:
            if (vtxp->nbargs.reduce_local.flags == -1 ||
                vtxp->nbargs.reduce_local.flags & MPIR_COLL_FLAG_REDUCE_L) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "Left reduction vtxp->nbargs.reduce_local.flags %ld \n",
                                 vtxp->nbargs.reduce_local.flags));
                MPIR_Reduce_local((const void *) vtxp->nbargs.reduce_local.inbuf,
                                  (void *) vtxp->nbargs.reduce_local.inoutbuf,
                                  vtxp->nbargs.reduce_local.count, vtxp->nbargs.reduce_local.dt,
                                  vtxp->nbargs.reduce_local.op);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "  --> GENERIC transport (reduce local_L) complete\n"));
            } else {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "Right reduction vtxp->nbargs.reduce_local.flags %ld \n",
                                 vtxp->nbargs.reduce_local.flags));
                /* An extra data copy is required to do the R reduction */
                MPIR_Reduce_local((const void *) vtxp->nbargs.reduce_local.inoutbuf,
                                  (void *) vtxp->nbargs.reduce_local.inbuf,
                                  vtxp->nbargs.reduce_local.count,
                                  vtxp->nbargs.reduce_local.dt, vtxp->nbargs.reduce_local.op);

                MPIR_Localcopy(vtxp->nbargs.reduce_local.inbuf,
                               vtxp->nbargs.reduce_local.count,
                               vtxp->nbargs.reduce_local.dt,
                               vtxp->nbargs.reduce_local.inoutbuf,
                               vtxp->nbargs.reduce_local.count, vtxp->nbargs.reduce_local.dt);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "  --> GENERIC transport (reduce local_R) complete\n"));
            }

            /* record vertex completion */
            MPII_COLL_GENERIC_record_vtx_completion(vtxp, sched);
            break;
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COLL_GENERIC_ISSUE_VTX);
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_free_buffers
/* Internal transport function to free any memory allocated for execution of this schedule */
void MPII_COLL_GENERIC_free_buffers(MPIR_COLL_GENERIC_sched_t * sched)
{
    int i;
    for (i = 0; i < sched->total; i++) {
        vtx_t *vtx = (vtx_t *) utarray_eltptr(sched->vtcs, i);
        /* free the temporary memory allocated by recv_reduce call */
        if (vtx->kind == MPIR_COLL_GENERIC_KIND_RECV_REDUCE) {
            MPL_free(vtx->nbargs.recv_reduce.inbuf);
            vtx->nbargs.recv_reduce.inbuf = NULL;
        }

        if (vtx->kind == MPIR_COLL_GENERIC_KIND_FREE_MEM) {
            MPL_free(vtx->nbargs.free_mem.ptr);
            vtx->nbargs.free_mem.ptr = NULL;
        }

        if (vtx->kind == MPIR_COLL_GENERIC_KIND_MULTICAST) {
            MPL_free(vtx->nbargs.multicast.req);
            MPL_free(vtx->nbargs.multicast.destinations);
        }
    }
    /* free temporary buffers allocated using MPIR_COLL_GENERIC_allocate_buffer call */
    for (i = 0; i < utarray_len(sched->buf_array); i++) {
        MPL_free(*(void **) utarray_eltptr(sched->buf_array, i));
    }
    utarray_free(sched->buf_array);

    utarray_free(sched->vtcs);
}

#undef FUNCNAME
#define FUNCNAME MPII_COLL_GENERIC_sched_destroy_fn
/* Internal transport function to destroy all memory associated with a schedule */
void MPII_COLL_GENERIC_sched_destroy_fn(MPIR_COLL_GENERIC_sched_t * sched)
{
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "In MPIR_COLL_GENERIC_sched_destroy_fn for schedule: %p\n",
                     sched));
    MPII_COLL_GENERIC_free_buffers(sched);
    MPL_free(sched);
}
