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
#include <sys/queue.h>
#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif


static inline int COLL_init()
{
    return 0;
}

static inline int COLL_comm_init(COLL_comm_t * comm, int id, int *tag_ptr, int rank, int comm_size)
{
    comm->id = id;
    TSP_comm_init(&comm->tsp_comm, COLL_COMM_BASE(comm));
    COLL_tree_comm_t *mycomm = &comm->tree_comm;
    int k = COLL_TREE_RADIX_DEFAULT;
    mycomm->curTag = 0;

    COLL_tree_init( rank, comm_size, k, 0, &mycomm->tree);
    char *e = getenv("COLL_DUMPTREE");

    if (e && atoi(e))
        COLL_tree_dump(mycomm->tree.nranks, 0, k);

    comm->tree_comm.curTag = tag_ptr;
    return 0;
}

static inline int COLL_comm_cleanup(COLL_comm_t * comm)
{
    TSP_comm_cleanup(&comm->tsp_comm);
    return 0;
}


static inline int COLL_allreduce(const void *sendbuf,
                                 void *recvbuf,
                                 int count,
                                 COLL_dt_t datatype,
                                 COLL_op_t op, COLL_comm_t * comm, int *errflag, int k)
{
    int rc;
    COLL_sched_t s;
    int tag = (*comm->tree_comm.curTag)++;

    COLL_sched_init(&s, tag);
    rc = COLL_sched_allreduce_tree(sendbuf, recvbuf, count, datatype, op, tag, comm, k, &s, 1);
    COLL_sched_kick(&s);
    return rc;
}


static inline int COLL_bcast(void *buffer,
                             int count,
                             COLL_dt_t datatype,
                             int root, COLL_comm_t * comm, int *errflag, int k, int segsize)
{
    int rc = 0;
    COLL_args_t coll_args = {.algo=COLL_NAME, .tsp=TRANSPORT_NAME, .nargs=7,\
            .args={.bcast={.buf=buffer,.count=count,.dt_id=(int)datatype,.root=root,.comm_id=comm->id,.k=k,.segsize=segsize}}};
    COLL_sched_t *s = MPIC_get_sched((MPIC_coll_args_t)coll_args);
    int tag = (*comm->tree_comm.curTag)++;
    if(s==NULL){
        if(0) fprintf(stderr, "schedule does not exist\n");
        s = (COLL_sched_t*)MPL_malloc(sizeof(COLL_sched_t));
        COLL_sched_init(s, tag);
        rc = COLL_sched_bcast_tree_pipelined(buffer, count, datatype, root, tag, comm, k, segsize, s, 1);
        MPIC_add_sched((MPIC_coll_args_t)coll_args, (void*)s, COLL_sched_free);
    } else{
        if(0) fprintf(stderr, "schedule already exists\n");
        COLL_sched_reset(s, tag);
    }
    COLL_sched_kick(s);
    return rc;
}


static inline int COLL_reduce(const void *sendbuf,
                              void *recvbuf,
                              int count,
                              COLL_dt_t datatype,
                              COLL_op_t op, int root, COLL_comm_t * comm, int *errflag, int k, int nbuffers)
{
    int rc;
    COLL_sched_t s;
    int tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init(&s, tag);
    rc = COLL_sched_reduce_tree_full(sendbuf, recvbuf, count, datatype, op, root, tag, comm, k, &s, 1, nbuffers);
    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_barrier(COLL_comm_t *comm,
                               int         *errflag,
                               int          k)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init(&s, tag);
    rc = COLL_sched_barrier_tree(tag, comm, k, &s);
    COLL_sched_kick(&s);
    return rc;
}


static inline int COLL_kick(COLL_queue_elem_t * elem)
{
    int done;
    COLL_sched_t *s = ((COLL_req_t *) elem)->phases;
    done = COLL_sched_kick_nb(s);

    if (done) {
        TAILQ_REMOVE(&COLL_progress_global.head, elem, list_data);
        TSP_sched_finalize(&s->tsp_sched);
        TSP_free_mem(s);
    }

    return done;
}
