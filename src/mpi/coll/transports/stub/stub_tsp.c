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
#include "stub_tsp.h"

int MPIR_COLL_STUB_init()
{
    return 0;
}

int MPIR_COLL_STUB_comm_init(MPIR_Comm * comm)
{
    return 0;
}

int MPIR_COLL_STUB_comm_init_null(MPIR_Comm * comm)
{
    return 0;
}

int MPIR_COLL_STUB_comm_cleanup(MPIR_Comm * comm)
{
    return 0;
}

int MPIR_COLL_STUB_sched_fence(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}

int MPIR_COLL_STUB_sched_wait(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}

int MPIR_COLL_STUB_sched_wait_for(MPIR_COLL_STUB_sched_t * sched, int nvtcs, int *vtcs)
{
    return 0;
}

int MPIR_COLL_STUB_sched_send(const void *buf, int count, MPI_Datatype dt, int dest, int tag,
                              MPIR_Comm * comm_ptr, MPIR_COLL_STUB_sched_t * sched,
                              int n_invtcs, int *invtcs)
{
    return 0;
}

int MPIR_COLL_STUB_sched_multicast(const void *buf, int count, MPI_Datatype dt, int *destinations,
                                   int num_destinations, int tag, MPIR_Comm * comm_ptr,
                                   MPIR_COLL_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

int MPIR_COLL_STUB_sched_send_accumulate(const void *buf, int count, MPI_Datatype dt,
                                         MPI_Op op, int dest, int tag,
                                         MPIR_Comm * comm_ptr, MPIR_COLL_STUB_sched_t * sched,
                                         int n_invtcs, int *invtcs)
{
    return 0;
}

int MPIR_COLL_STUB_sched_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                              MPIR_Comm * comm_ptr, MPIR_COLL_STUB_sched_t * sched,
                              int n_invtcs, int *invtcs)
{
    return 0;
}

int MPIR_COLL_STUB_sched_recv_reduce(void *buf, int count, MPI_Datatype datatype,
                                     MPI_Op op, int source, int tag,
                                     MPIR_Comm * comm_ptr, uint64_t flags,
                                     MPIR_COLL_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

int MPIR_COLL_STUB_sched_poke(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}

int MPIR_COLL_STUB_sched_reduce_local(const void *inbuf, void *inoutbuf, int count,
                                      MPI_Datatype datatype, MPI_Op operation,
                                      uint64_t flags, MPIR_COLL_STUB_sched_t * sched, int nvtcs,
                                      int *vtcs)
{
    return 0;
}

int MPIR_COLL_STUB_sched_dt_copy(void *tobuf, int tocount, MPI_Datatype totype,
                                 const void *frombuf, int fromcount, MPI_Datatype fromtype,
                                 MPIR_COLL_STUB_sched_t * sched, int n_invtcs, int *invtcs)
{
    return 0;
}

int MPIR_COLL_STUB_sched_free_mem(void *ptr, MPIR_COLL_STUB_sched_t * sched, int n_invtcs,
                                  int *invtcs)
{
    return 0;
}

MPIR_COLL_STUB_sched_t *MPIR_COLL_STUB_get_schedule(MPIR_Comm * comm_ptr, void *key,
                                                    int key_len, int tag, int *is_new)
{
    *is_new = 1;
    return NULL;
}

void MPIR_COLL_STUB_save_schedule(MPIR_Comm * comm_ptr, void *key,
                                  int key_len, MPIR_COLL_STUB_sched_t * s)
{
}


void *MPIR_COLL_STUB_allocate_buffer(size_t size, MPIR_COLL_STUB_sched_t * s)
{
    return NULL;
}

void MPIR_COLL_STUB_free_buffers(MPIR_COLL_STUB_sched_t * s)
{
}

int MPIR_COLL_STUB_sched_commit(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}

int MPIR_COLL_STUB_sched_finalize(MPIR_COLL_STUB_sched_t * sched)
{
    return 0;
}
