/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif



static inline int COLL_init()
{
    TSP_init_control_dt(&COLL_global.control_dt);
    return 0;
}

static inline int COLL_comm_init(COLL_comm_t *comm, int *tag_ptr)
{
    COLL_tree_comm_t *mycomm = &comm->tree_comm;
    int               k      = COLL_TREE_RADIX_DEFAULT;
    mycomm->curTag           = 0;

    COLL_tree_init(TSP_rank(&comm->tsp_comm),
                   TSP_size(&comm->tsp_comm),
                   k,
                   0,
                   &mycomm->tree);
    char *e = getenv("COLL_DUMPTREE");

    if(e && atoi(e))
      COLL_tree_dump(mycomm->tree.nranks, 0, k);

    comm->tree_comm.curTag      = tag_ptr;
    return 0;
}

static inline int COLL_allgather(const void  *sendbuf,
                                 int          sendcount,
                                 COLL_dt_t   *sendtype,
                                 void        *recvbuf,
                                 int          recvcount,
                                 COLL_dt_t   *recvtype,
                                 COLL_comm_t *comm,
                                 int         *errflag,
                                 int          k)
{
    int                rc;
    COLL_sched_t       s;
    int                tag = (*comm->tree_comm.curTag)++;

    COLL_sched_init(&s);
    rc = COLL_sched_allgather_ring(sendbuf,sendcount,sendtype,
                                   recvbuf,recvcount,recvtype,tag,comm,&s,1);

    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_allgatherv(const void  *sendbuf,
                                  int          sendcount,
                                  COLL_dt_t   *sendtype,
                                  void        *recvbuf,
                                  const int   *recvcounts,
                                  const int   *displs,
                                  COLL_dt_t   *recvtype,
                                  COLL_comm_t *comm,
                                  int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_allreduce(const void  *sendbuf,
                                 void        *recvbuf,
                                 int          count,
                                 COLL_dt_t   *datatype,
                                 COLL_op_t   *op,
                                 COLL_comm_t *comm,
                                 int         *errflag,
                                 int          k)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->tree_comm.curTag)++;

    COLL_sched_init(&s);
    rc = COLL_sched_allreduce(sendbuf,recvbuf,count,
                              datatype,op,tag,comm,k,&s,1);
    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_alltoall(const void  *sendbuf,
                                int          sendcount,
                                COLL_dt_t   *sendtype,
                                void        *recvbuf,
                                int          recvcount,
                                COLL_dt_t   *recvtype,
                                COLL_comm_t *comm,
                                int         *errflag)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->tree_comm.curTag)++;

    COLL_sched_init(&s);
    /*    rc = COLL_sched_alltoall_scattered(sendbuf,sendcount,sendtype,
                                  recvbuf,recvcount,recvtype,tag,comm,&s,1);
        rc = COLL_sched_alltoall_pairwise(sendbuf,sendcount,sendtype,
                                  recvbuf,recvcount,recvtype,tag,comm,&s,1);
      */rc = COLL_sched_alltoall_ring(sendbuf,sendcount,sendtype,
                                      recvbuf,recvcount,recvtype,tag,comm,&s,1);

    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_alltoallv(const void  *sendbuf,
                                 const int   *sendcnts,
                                 const int   *sdispls,
                                 COLL_dt_t   *sendtype,
                                 void        *recvbuf,
                                 const int   *recvcnts,
                                 const int   *rdispls,
                                 COLL_dt_t   *recvtype,
                                 COLL_comm_t *comm,
                                 int         *errflag)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->tree_comm.curTag)++;

    COLL_sched_init(&s);
    rc = COLL_sched_alltoallv_scattered(sendbuf,sendcnts,sdispls, sendtype,
                                        recvbuf,recvcnts,rdispls,recvtype,tag,comm,&s,1);

    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_alltoallw(const void       *sendbuf,
                                 const int        *sendcnts,
                                 const int        *sdispls,
                                 const COLL_dt_t **sendtypes,
                                 void             *recvbuf,
                                 const int        *recvcnts,
                                 const int        *rdispls,
                                 const COLL_dt_t **recvtypes,
                                 COLL_comm_t      *comm,
                                 int              *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_bcast(void        *buffer,
                             int          count,
                             COLL_dt_t   *datatype,
                             int          root,
                             COLL_comm_t *comm,
                             int         *errflag,
                             int          k)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init(&s);
    rc = COLL_sched_bcast(buffer,count,datatype,root,tag,comm,k,&s,1);
    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_exscan(const void  *sendbuf,
                              void        *recvbuf,
                              int          count,
                              COLL_dt_t   *datatype,
                              COLL_op_t   *op,
                              COLL_comm_t *comm,
                              int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_gather(const void  *sendbuf,
                              int          sendcnt,
                              COLL_dt_t    sendtype,
                              void        *recvbuf,
                              int          recvcnt,
                              COLL_dt_t   *recvtype,
                              int          root,
                              COLL_comm_t *comm,
                              int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_gatherv(const void  *sendbuf,
                               int          sendcnt,
                               COLL_dt_t   *sendtype,
                               void        *recvbuf,
                               const int   *recvcnts,
                               const int   *displs,
                               COLL_dt_t   *recvtype,
                               int          root,
                               COLL_comm_t *comm,
                               int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_reduce_scatter(const void  *sendbuf,
                                      void        *recvbuf,
                                      const int   *recvcnts,
                                      COLL_dt_t    datatype,
                                      COLL_op_t    op,
                                      COLL_comm_t  comm,
                                      int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_reduce_scatter_block(const void  *sendbuf,
                                            void        *recvbuf,
                                            int          recvcount,
                                            COLL_dt_t   *datatype,
                                            COLL_op_t   *op,
                                            COLL_comm_t *comm,
                                            int         *errflag)
{
    assert(0);
    return 0;
}


static inline int COLL_reduce(const void  *sendbuf,
                              void        *recvbuf,
                              int          count,
                              COLL_dt_t   *datatype,
                              COLL_op_t   *op,
                              int          root,
                              COLL_comm_t *comm,
                              int         *errflag,
                              int          k)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init(&s);
    rc = COLL_sched_reduce_full(sendbuf,recvbuf,count,datatype,
                                op,root,tag,comm,k,&s,1);
    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_scan(const void  *sendbuf,
                            void        *recvbuf,
                            int         count,
                            COLL_dt_t   *datatype,
                            COLL_op_t   *op,
                            COLL_comm_t *comm,
                            int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_scatter(const void  *sendbuf,
                               int          sendcnt,
                               COLL_dt_t   *sendtype,
                               void        *recvbuf,
                               int          recvcnt,
                               COLL_dt_t   *recvtype,
                               int          root,
                               COLL_comm_t *comm,
                               int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_scatterv(const void  *sendbuf,
                                const int   *sendcnts,
                                const int   *displs,
                                COLL_dt_t   *sendtype,
                                void        *recvbuf,
                                int          recvcnt,
                                COLL_dt_t   *recvtype,
                                int          root,
                                COLL_comm_t *comm,
                                int         *errflag)
{
    assert(0);
    return 0;
}

static inline int COLL_barrier(COLL_comm_t *comm,
                               int         *errflag,
                               int          k)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init(&s);
    rc = COLL_sched_barrier(tag, comm, k, &s);
    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_iallgather(const void  *sendbuf,
                                  int          sendcount,
                                  COLL_dt_t   *sendtype,
                                  void        *recvbuf,
                                  int          recvcount,
                                  COLL_dt_t   *recvtype,
                                  COLL_comm_t *comm,
                                  COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_iallgatherv(const void  *sendbuf,
                                   int          sendcount,
                                   COLL_dt_t   *sendtype,
                                   void        *recvbuf,
                                   const int   *recvcounts,
                                   const int   *displs,
                                   COLL_dt_t   *recvtype,
                                   COLL_comm_t *comm,
                                   COLL_req_t  *request)
{
    assert(0);
    return 0;
}


static inline int COLL_kick(COLL_queue_elem_t *elem)
{
    int                 done;
    COLL_sched_t *s    = ((COLL_req_t *)elem)->phases;
    done = COLL_sched_kick_nb(s);

    if(done) {
        TAILQ_REMOVE(&COLL_progress_global.head, elem, list_data);
        TSP_sched_finalize(&s->tsp_sched);
        TSP_free_mem(s);
    }

    return done;
}

static inline int COLL_iallreduce(const void  *sendbuf,
                                  void        *recvbuf,
                                  int          count,
                                  COLL_dt_t   *datatype,
                                  COLL_op_t   *op,
                                  COLL_comm_t *comm,
                                  COLL_req_t  *request,
                                  int          k)
{
#if 0
    COLL_sched_t  *s;
    int                  done = 0;
    int                  tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init_nb(&s,request);
    TSP_addref_op(&op->tsp_op,1);
    TSP_addref_dt(&datatype->tsp_dt,1);
    COLL_sched_allreduce(sendbuf,recvbuf,count,datatype,
                         op,tag,comm,k,s,0);
    TSP_fence(&s->tsp_sched);
    TSP_addref_op_nb(&op->tsp_op,0,&s->tsp_sched);
    TSP_addref_dt_nb(&datatype->tsp_dt,0,&s->tsp_sched);

    TSP_fence(&s->tsp_sched);
    TSP_sched_commit(&s->tsp_sched);

    done = COLL_sched_kick_nb(s);

    if(1 || !done) {
        TAILQ_INSERT_TAIL(&COLL_progress_global.head,&request->elem,list_data);
    } else
        TSP_free_mem(s);
#endif
    return 0;
}

static inline int COLL_ialltoall(const void  *sendbuf,
                                 int          sendcount,
                                 COLL_dt_t   *sendtype,
                                 void        *recvbuf,
                                 int          recvcount,
                                 COLL_dt_t   *recvtype,
                                 COLL_comm_t *comm,
                                 COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ialltoallv(const void  *sendbuf,
                                  const int   *sendcnts,
                                  const int   *sdispls,
                                  COLL_dt_t    sendtype,
                                  void        *recvbuf,
                                  const int   *recvcnts,
                                  const int   *rdispls,
                                  COLL_dt_t    recvtype,
                                  COLL_comm_t  comm,
                                  COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ialltoallw(const void       *sendbuf,
                                  const int        *sendcnts,
                                  const int        *sdispls,
                                  const COLL_dt_t **sendtypes,
                                  void             *recvbuf,
                                  const int        *recvcnts,
                                  const int        *rdispls,
                                  const COLL_dt_t **recvtypes,
                                  COLL_comm_t      *comm,
                                  COLL_req_t       *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ibcast(void        *buffer,
                              int          count,
                              COLL_dt_t   *datatype,
                              int          root,
                              COLL_comm_t *comm,
                              COLL_req_t  *request,
                              int          k)
{
#if 0
    COLL_sched_t  *s;
    int                  done = 0;
    int                  tag  = (*comm->tree_comm.curTag)++;
    COLL_sched_init_nb(&s,request);
    TSP_addref_dt(&datatype->tsp_dt,1);
    COLL_sched_bcast(buffer,count,datatype,
                     root,tag,comm,k,s,0);

    TSP_addref_dt_nb(&datatype->tsp_dt,0,&s->tsp_sched);
    TSP_fence(&s->tsp_sched);
    TSP_sched_commit(&s->tsp_sched);

    done = COLL_sched_kick_nb(s);

    if(1 || !done) { /* always enqueue until we can fix the request interface */
        TAILQ_INSERT_TAIL(&COLL_progress_global.head,&request->elem,list_data);
    } else
        TSP_free_mem(s);
#endif
    return 0;
}

static inline int COLL_iexscan(const void  *sendbuf,
                               void        *recvbuf,
                               int          count,
                               COLL_dt_t   *datatype,
                               COLL_op_t   *op,
                               COLL_comm_t *comm,
                               COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_igather(const void  *sendbuf,
                               int          sendcnt,
                               COLL_dt_t   *sendtype,
                               void        *recvbuf,
                               int          recvcnt,
                               COLL_dt_t   *recvtype,
                               int          root,
                               COLL_comm_t *comm,
                               COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_igatherv(const void  *sendbuf,
                                int          sendcnt,
                                COLL_dt_t   *sendtype,
                                void        *recvbuf,
                                const int   *recvcnts,
                                const int   *displs,
                                COLL_dt_t   *recvtype,
                                int          root,
                                COLL_comm_t *comm,
                                COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ireduce_scatter(const void  *sendbuf,
                                       void        *recvbuf,
                                       const int   *recvcnts,
                                       COLL_dt_t   *datatype,
                                       COLL_op_t   *op,
                                       COLL_comm_t *comm,
                                       COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ireduce_scatter_block(const void  *sendbuf,
                                             void        *recvbuf,
                                             int          recvcount,
                                             COLL_dt_t   *datatype,
                                             COLL_op_t   *op,
                                             COLL_comm_t *comm,
                                             COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ireduce(const void  *sendbuf,
                               void        *recvbuf,
                               int          count,
                               COLL_dt_t   *datatype,
                               COLL_op_t   *op,
                               int          root,
                               COLL_comm_t *comm,
                               COLL_req_t  *request,
                               int          k)
{
#if 0
    COLL_sched_t  *s;
    int                  done = 0;
    int                  tag  = (*comm->tree_comm.curTag)++;
    COLL_sched_init_nb(&s,request);
    TSP_addref_op(&op->tsp_op,1);
    TSP_addref_dt(&datatype->tsp_dt,1);
    COLL_sched_reduce_full(sendbuf,recvbuf,count,datatype,
                           op,root,tag,comm,k,s,0);
    TSP_addref_op_nb(&op->tsp_op,0,&s->tsp_sched);
    TSP_addref_dt_nb(&datatype->tsp_dt,0,&s->tsp_sched);
    TSP_fence(&s->tsp_sched);
    TSP_sched_commit(&s->tsp_sched);

    done = COLL_sched_kick_nb(s);

    if(1 || !done) { /* always enqueue until we can fix the request interface */
        TAILQ_INSERT_TAIL(&COLL_progress_global.head,&request->elem,list_data);
    } else
        TSP_free_mem(s);
#endif
    return 0;
}

static inline int COLL_iscan(const void  *sendbuf,
                             void        *recvbuf,
                             int          count,
                             COLL_dt_t   *datatype,
                             COLL_op_t   *op,
                             COLL_comm_t *comm,
                             COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_iscatter(const void  *sendbuf,
                                int          sendcnt,
                                COLL_dt_t   *sendtype,
                                void        *recvbuf,
                                int          recvcnt,
                                COLL_dt_t   *recvtype,
                                int          root,
                                COLL_comm_t *comm,
                                COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_iscatterv(const void  *sendbuf,
                                 const int   *sendcnts,
                                 const int   *displs,
                                 COLL_dt_t   *sendtype,
                                 void        *recvbuf,
                                 int          recvcnt,
                                 COLL_dt_t   *recvtype,
                                 int          root,
                                 COLL_comm_t *comm,
                                 COLL_req_t  *request)
{
    assert(0);
    return 0;
}

static inline int COLL_ibarrier(COLL_comm_t *comm,
                                COLL_req_t  *request,
                                int          k)
{
    COLL_sched_t  *s;
    int                  done = 0;
    int                  tag  = (*comm->tree_comm.curTag)++;
    COLL_sched_init_nb(&s,request);
    COLL_sched_barrier(tag,comm,k,s);
    done = COLL_sched_kick_nb(s);

    if(1 || !done) { /* always enqueue until we can fix the request interface */
        TAILQ_INSERT_TAIL(&COLL_progress_global.head,&request->elem,list_data);
    } else
        TSP_free_mem(s);

    return 0;
}

static inline int COLL_neighbor_allgather(const void  *sendbuf,
                                          int          sendcount,
                                          COLL_dt_t   *sendtype,
                                          void        *recvbuf,
                                          int          recvcount,
                                          COLL_dt_t   *recvtype,
                                          COLL_comm_t *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_neighbor_allgatherv(const void  *sendbuf,
                                           int          sendcount,
                                           COLL_dt_t   *sendtype,
                                           void        *recvbuf,
                                           const int    recvcounts[],
                                           const int    displs[],
                                           COLL_dt_t   *recvtype,
                                           COLL_comm_t *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_neighbor_alltoall(const void  *sendbuf,
                                         int          sendcount,
                                         COLL_dt_t   *sendtype,
                                         void        *recvbuf,
                                         int          recvcount,
                                         COLL_dt_t   *recvtype,
                                         COLL_comm_t *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_neighbor_alltoallv(const void  *sendbuf,
                                          const int    sendcounts[],
                                          const int    sdispls[],
                                          COLL_dt_t   *sendtype,
                                          void        *recvbuf,
                                          const int    recvcounts[],
                                          const int    rdispls[],
                                          COLL_dt_t   *recvtype,
                                          COLL_comm_t *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_neighbor_alltoallw(const void        *sendbuf,
                                          const int          sendcounts[],
                                          const COLL_aint_t *sdispls[],
                                          const COLL_dt_t   *sendtypes[],
                                          void              *recvbuf,
                                          const int          recvcounts[],
                                          const COLL_aint_t *rdispls[],
                                          const COLL_dt_t   *recvtypes[],
                                          COLL_comm_t       *comm)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_allgather(const void   *sendbuf,
                                           int           sendcount,
                                           COLL_dt_t    *sendtype,
                                           void         *recvbuf,
                                           int           recvcount,
                                           COLL_dt_t    *recvtype,
                                           COLL_comm_t  *comm,
                                           COLL_sched_t *s)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_allgatherv(const void   *sendbuf,
                                            int           sendcount,
                                            COLL_dt_t    *sendtype,
                                            void         *recvbuf,
                                            const int     recvcounts[],
                                            const int     displs[],
                                            COLL_dt_t    *recvtype,
                                            COLL_comm_t  *comm,
                                            COLL_sched_t *s)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_alltoall(const void   *sendbuf,
                                          int           sendcount,
                                          COLL_dt_t    *sendtype,
                                          void         *recvbuf,
                                          int           recvcount,
                                          COLL_dt_t    *recvtype,
                                          COLL_comm_t  *comm,
                                          COLL_sched_t *s)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_alltoallv(const void   *sendbuf,
                                           const int     sendcounts[],
                                           const int     sdispls[],
                                           COLL_dt_t    *sendtype,
                                           void         *recvbuf,
                                           const int     recvcounts[],
                                           const int     rdispls[],
                                           COLL_dt_t    *recvtype,
                                           COLL_comm_t  *comm,
                                           COLL_sched_t *s)
{
    assert(0);
    return 0;
}

static inline int COLL_ineighbor_alltoallw(const void        *sendbuf,
                                           const int          sendcounts[],
                                           const COLL_aint_t *sdispls[],
                                           const COLL_dt_t   *sendtypes[],
                                           void              *recvbuf,
                                           const int          recvcounts[],
                                           const COLL_aint_t *rdispls[],
                                           const COLL_dt_t   *recvtypes[],
                                           COLL_comm_t       *comm,
                                           COLL_sched_t      *s)
{
    assert(0);
    return 0;
}
