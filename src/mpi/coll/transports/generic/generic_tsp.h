/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef GENERIC_TRANSPORT_H_INCLUDED
#define GENERIC_TRANSPORT_H_INCLUDED

/* Transport initialization function */
int MPIR_COLL_GENERIC_init(void);

/* Transport function to store a schedule */
void MPIR_COLL_GENERIC_save_schedule(MPIR_Comm * comm_ptr, void *key, int len, void *s);

/* Transport function to initialize a new schedule */
void MPIR_COLL_GENERIC_sched_init(MPIR_COLL_GENERIC_sched_t * sched, int tag);

/* Transport function to reset an existing schedule */
void MPIR_COLL_GENERIC_sched_reset(MPIR_COLL_GENERIC_sched_t * sched, int tag);

/* Transport function to get schedule (based on the key) if it exists, else return a new schedule */
MPIR_COLL_GENERIC_sched_t *MPIR_COLL_GENERIC_get_schedule(MPIR_Comm * comm_ptr,
                                                          void *key, int len, int tag, int *is_new);

/* Transport function to tell that the schedule generation is now complete */
void MPIR_COLL_GENERIC_sched_commit(MPIR_COLL_GENERIC_sched_t * sched);

/* Transport function to tell that the schedule can now be freed
 * and can also be removed from the database. We are not doing
 * anything here because the schedule is stored in the database
 * can be reused in future */
void MPIR_COLL_GENERIC_sched_finalize(MPIR_COLL_GENERIC_sched_t * sched);

/* Transport function that adds a no op vertex in the graph that has
 * all the vertices posted before it as incoming vertices */
int MPIR_COLL_GENERIC_sched_fence(MPIR_COLL_GENERIC_sched_t * sched);

/* Transport function that adds a no op vertex in the graph with specified dependencies */
int MPIR_COLL_GENERIC_sched_wait_for(MPIR_COLL_GENERIC_sched_t * sched, int nvtcs, int *vtcs);

/* MPIR_COLL_GENERIC_wait waits for all the operations posted before it to complete
* before issuing any operations posted after it. This is useful in composing
* multiple schedules, for example, allreduce can be written as
* MPIR_TSP_sched_reduce(s)
* MPIR_TSP_wait(s)
* MPIR_TSP_sched_bcast(s)
* This is different from the fence operation in the sense that fence requires
* a vertex to post dependencies on it while MPIR_COLL_GENERIC_wait is used internally
* by the transport to add it as a dependency to any operations poster after it
*/
int MPIR_COLL_GENERIC_sched_wait(MPIR_COLL_GENERIC_sched_t * sched);

/* Transport function to schedule a send task */
int MPIR_COLL_GENERIC_sched_send(const void *buf,
                                 int count,
                                 MPI_Datatype dt,
                                 int dest,
                                 int tag,
                                 MPIR_Comm * comm_ptr,
                                 MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs);

int MPIR_COLL_GENERIC_sched_multicast(const void *buf,
                                      int count,
                                      MPI_Datatype dt,
                                      int *destinations,
                                      int num_destinations,
                                      int tag,
                                      MPIR_Comm * comm_ptr,
                                      MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs);

/* Transport function to schedule a send_accumulate task */
int MPIR_COLL_GENERIC_sched_send_accumulate(const void *buf,
                                            int count,
                                            MPI_Datatype dt,
                                            MPI_Op op,
                                            int dest,
                                            int tag,
                                            MPIR_Comm * comm_ptr,
                                            MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs,
                                            int *invtcs);

/* Transport function to schedule a recv task */
int MPIR_COLL_GENERIC_sched_recv(void *buf,
                                 int count,
                                 MPI_Datatype dt,
                                 int source,
                                 int tag,
                                 MPIR_Comm * comm_ptr,
                                 MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs);

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
                                        int *invtcs);

/* Transport function to schedule a local reduce */
int MPIR_COLL_GENERIC_sched_reduce_local(const void *inbuf,
                                         void *inoutbuf,
                                         int count,
                                         MPI_Datatype datatype,
                                         MPI_Op operation,
                                         uint64_t flags,
                                         MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs,
                                         int *invtcs);

/* Transport function to schedule a data copy */
int MPIR_COLL_GENERIC_sched_dt_copy(void *tobuf,
                                    int tocount,
                                    MPI_Datatype totype,
                                    const void *frombuf,
                                    int fromcount,
                                    MPI_Datatype fromtype,
                                    MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs);;

/* Transport function to allocate memory required for schedule execution.
 *The memory address is recorded by the transport in the schedule
 * so that this memory can be freed when the schedule is destroyed
 */
void *MPIR_COLL_GENERIC_allocate_buffer(size_t size, MPIR_COLL_GENERIC_sched_t * s);

/* Transport function to schedule memory free'ing */
int MPIR_COLL_GENERIC_sched_free_mem(void *ptr,
                                     MPIR_COLL_GENERIC_sched_t * sched, int n_invtcs, int *invtcs);

/* Transport function to make progress on the schedule */
int MPIR_COLL_GENERIC_sched_poke(MPIR_COLL_GENERIC_sched_t * sched, bool * is_complete);

/* Transport function to initialize a transport communicator */
int MPIR_COLL_GENERIC_comm_init(MPIR_Comm * comm_ptr);

/* Transport function to initialize a transport communicator data to NULL */
int MPIR_COLL_GENERIC_comm_init_null(MPIR_Comm * comm_ptr);

/* Transport function to cleanup a transport communicator */
int MPIR_COLL_GENERIC_comm_cleanup(MPIR_Comm * comm_ptr);

/* Transport function to wait for a schedule to complete execution, used for blocking
 * collectives */
int MPIR_COLL_GENERIC_wait_sched(MPIR_COLL_GENERIC_sched_t * sched);

/* Transport function to make progress on a non-blocking collective */
int MPIR_COLL_GENERIC_progress_nbc(MPIR_COLL_req_t * coll_req, bool * done);

/* Transport function to enqueue and kick start a non-blocking collective */
int MPIR_COLL_GENERIC_sched_start(MPIR_COLL_GENERIC_sched_t * sched, MPIR_Comm * comm,
                                  MPIR_Request ** request);

/* Progress hook for non blocking collectives */
int MPIR_COLL_GENERIC_progress_hook(int *);

/* Tells if there are any pending schedules */
bool MPIR_COLL_GENERIC_sched_are_pending();

#endif
