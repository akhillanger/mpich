/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include "utarray.h"

#ifndef GENERIC_TRANSPORT_TYPES_H_INCLUDED
#define GENERIC_TRANSPORT_TYPES_H_INCLUDED

/* Maximum number of incoming/outgoing edges in the collective schedule graph.
 * This is used only as an initial hint to allocate memory to store edges */
#undef MPIR_COLL_GENERIC_MAX_EDGES
#define MPIR_COLL_GENERIC_MAX_EDGES 1

/* Maximum number of tasks in the collective schedule graph.
 * This is used only as an initial hint to allocate memory to store tasks */
#undef MPIR_COLL_GENERIC_MAX_TASKS
#define MPIR_COLL_GENERIC_MAX_TASKS 1

/* Data structure to store send or recv arguments */
typedef struct {
    void *buf;
    int count;
    MPI_Datatype dt;
    /* Stores destination in case of send and source in case of recv call */
    int dest;
    MPIR_Comm *comm;
} MPIR_COLL_GENERIC_sendrecv_arg_t;

typedef struct {
    void *buf;
    int count;
    MPI_Datatype dt;
    /*array of ranks to send the data to */
    int *destinations;
    int num_destinations;
    MPIR_Comm *comm;

    /*some data structures to keep track of the progress */
    /*request array */
    MPIR_Request **req;
    /*last send that has completed */
    int last_complete;

} MPIR_COLL_GENERIC_multicast_arg_t;

/* Data structure to store recv_reduce arguments */
typedef struct {
    void *inbuf;
    void *inoutbuf;
    int count;
    MPI_Datatype datatype;
    MPI_Op op;
    int source;
    MPIR_Comm *comm;
    int done;
    uint64_t flags;
    /* also stored pointer to the vertex of the recv_reduce in the graph */
    struct MPIR_COLL_GENERIC_vtx_t *vtxp;
} MPIR_COLL_GENERIC_recv_reduce_arg_t;

/* Data structure to store dt_copy arguments */
typedef struct {
    void *tobuf;
    int tocount;
    MPI_Datatype totype;
    const void *frombuf;
    int fromcount;
    MPI_Datatype fromtype;
} MPIR_COLL_GENERIC_dt_copy_arg_t;

/* Data structure to store reduce_local arguments */
typedef struct {
    const void *inbuf;
    void *inoutbuf;
    int count;
    MPI_Datatype dt;
    MPI_Op op;
    /* Flag to tell whether the reduction is inbuf+inoutbuf or inoutbuf+inbuf */
    uint64_t flags;
} MPIR_COLL_GENERIC_reduce_local_arg_t;

/* Data structure to store free_mem arguments */
typedef struct {
    void *ptr;
} MPIR_COLL_GENERIC_free_mem_arg_t;

/* Enumerator for various transport functions */
typedef enum {
    MPIR_COLL_GENERIC_KIND_SEND,
    MPIR_COLL_GENERIC_KIND_RECV,
    MPIR_COLL_GENERIC_KIND_MULTICAST,
    MPIR_COLL_GENERIC_KIND_DT_COPY,
    MPIR_COLL_GENERIC_KIND_FREE_MEM,
    MPIR_COLL_GENERIC_KIND_RECV_REDUCE,
    MPIR_COLL_GENERIC_KIND_REDUCE_LOCAL,
    MPIR_COLL_GENERIC_KIND_NOOP,
    MPIR_COLL_GENERIC_KIND_FENCE,
    MPIR_COLL_GENERIC_KIND_WAIT
} MPIR_COLL_GENERIC_TASK_KIND;

/* Enumerator for various states of a task */
typedef enum {
    /* Task is initialized, but not issued.
     * It remains in this state while waiting for
     * the dependencies to complete */
    MPIR_COLL_GENERIC_STATE_INIT,
    /* Task has been issued but has not completed execution yet */
    MPIR_COLL_GENERIC_STATE_ISSUED,
    /* Task has completed */
    MPIR_COLL_GENERIC_STATE_COMPLETE,
} MPIR_COLL_GENERIC_TASK_STATE;

/* Data structure for a vertex in the graph
 * Each vertex corresponds to a task.
 * NOTE: Make sure to update its copy and destructor functions whenever
 * a change is made to this data structure */
typedef struct MPIR_COLL_GENERIC_vtx_t {
    /* Kind of the task associated with this vertex: enum MPIR_COLL_GENERIC_TASK_KIND */
    int kind;
    /* Current state of the task associated with this vertex: enum MPIR_COLL_GENERIC_TASK_STATE */
    int state;
    /* request pointers for the task, recv_reduce task will need two request pointers */
    MPIR_Request *req[2];
    /* a unique id for this vertex */
    int id;

    /* Integer arrays of incoming and outgoing vertices */
    UT_array *invtcs;
    UT_array *outvtcs;

    int num_unfinished_dependencies;
    /* Union to store task arguments depending on the task type */
    union {
        MPIR_COLL_GENERIC_sendrecv_arg_t sendrecv;
        MPIR_COLL_GENERIC_multicast_arg_t multicast;
        MPIR_COLL_GENERIC_dt_copy_arg_t dt_copy;
        MPIR_COLL_GENERIC_free_mem_arg_t free_mem;
        MPIR_COLL_GENERIC_recv_reduce_arg_t recv_reduce;
        MPIR_COLL_GENERIC_reduce_local_arg_t reduce_local;
    } nbargs;

    /* This transport maintains a linked list of issued vertices. If this vertex is
     * currently issued (that is, state == MPIR_COLL_GENERIC_STATE_ISSUED), next_issued points
     * to the next issued vertex in the liked list. Else it is set to NULL */
    struct MPIR_COLL_GENERIC_vtx_t *next_issued;
} MPIR_COLL_GENERIC_vtx_t;

/* Data structure to store schedule of a collective operation */
typedef struct {
    /* Tag value to be used by the tasks in this schedule */
    int tag;
    /* Array of vertices */
    UT_array *vtcs;
    /* Total number of vertices */
    int total;
    /* Of the total vertices, number of vertices that have completed
     * This is reset to zero whenever the schedule is reused */
    int num_completed;
    /* Maximum number of vertices that can be stored in the vtcs array. This is increased
     * when more vertices need to be added */
    int max_vtcs;
    /* Maximum number of edges per vertex. This is increase when more edges need to be added */
    int max_edges_per_vtx;

    /* last_wait keeps track of the vertex id of the last MPIR_TSP_wait call
     * so that when the next MPIR_TSP_wait call is made, its incoming dependencies
     * are made only up to the last_wait vertex to avoid redundant dependencies */
    int last_wait;


    /* Store the memory location of all the buffers that were temporarily
     * allocated to execute the schedule. This information is later used to
     * free those memory locations when the schedule is destroyed
     * (MPIR_COLL_GENERIC_free_sched_mem) Note that the temporary memory allocated by
     * recv_reduce is not recorded here since the transport already knows about it
     */

    UT_array *buf_array;

    struct MPIR_COLL_GENERIC_vtx_t *issued_head;        /* head of the issued vertices linked list */
    struct MPIR_COLL_GENERIC_vtx_t *vtx_iter;   /* temporary pointer to keep track of the
                                                 * current vertex under consideration in
                                                 * MPIR_TSP_test function */
    struct MPIR_COLL_GENERIC_vtx_t *last_issued;        /* temporary pointer to the last vertex
                                                         * issued in the current pass of MPIR_TSP_test
                                                         * function */
} MPIR_COLL_GENERIC_sched_t;

typedef MPIR_COLL_GENERIC_vtx_t vtx_t;

#endif /* GENERIC_TRANSPORT_TYPES_H_INCLUDED */
