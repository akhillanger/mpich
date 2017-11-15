/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Transport data structures */
#define MPIR_TSP_sched_t                  MPIR_COLL_GENERIC_sched_t

/* General transport API */
#define MPIR_TSP_init                     MPIR_COLL_GENERIC_init
#define MPIR_TSP_comm_cleanup             MPIR_COLL_GENERIC_comm_cleanup
#define MPIR_TSP_comm_init                MPIR_COLL_GENERIC_comm_init
#define MPIR_TSP_comm_init_null           MPIR_COLL_GENERIC_comm_init_null
#define MPIR_TSP_sched_fence              MPIR_COLL_GENERIC_sched_fence
#define MPIR_TSP_sched_wait               MPIR_COLL_GENERIC_sched_wait
#define MPIR_TSP_sched_wait_for           MPIR_COLL_GENERIC_sched_wait_for
#define MPIR_TSP_sched_dt_info            MPIR_COLL_GENERIC_sched_dt_info
#define MPIR_TSP_sched_send               MPIR_COLL_GENERIC_sched_send
#define MPIR_TSP_sched_recv               MPIR_COLL_GENERIC_sched_recv
#define MPIR_TSP_sched_multicast          MPIR_COLL_GENERIC_sched_multicast
#define MPIR_TSP_sched_recv_reduce        MPIR_COLL_GENERIC_sched_recv_reduce
#define MPIR_TSP_sched_send_accumulate    MPIR_COLL_GENERIC_sched_send_accumulate
#define MPIR_TSP_sched_poke               MPIR_COLL_GENERIC_sched_poke
#define MPIR_TSP_sched_reduce_local       MPIR_COLL_GENERIC_sched_reduce_local
#define MPIR_TSP_sched_dt_copy            MPIR_COLL_GENERIC_sched_dt_copy
#define MPIR_TSP_allocate_buffer          MPIR_COLL_GENERIC_allocate_buffer
#define MPIR_TSP_sched_free_mem           MPIR_COLL_GENERIC_sched_free_mem
#define MPIR_TSP_free_buffers             MPIR_COLL_GENERIC_free_buffers
#define MPIR_TSP_wait_sched               MPIR_COLL_GENERIC_wait_sched
#define MPIR_TSP_progress_nbc             MPIR_COLL_GENERIC_progress_nbc
#define MPIR_TSP_sched_start              MPIR_COLL_GENERIC_sched_start

/* Transport API for collective schedules */

/* Get schedule from database. Returns NULL if schedule not present
 * else reset schedule and return the schedule */
#define MPIR_TSP_get_schedule             MPIR_COLL_GENERIC_get_schedule
/* Save schedule in the database */
#define MPIR_TSP_save_schedule            MPIR_COLL_GENERIC_save_schedule
/* Release schedule after use (release all associated memory if
 * it is not being stored in database for future use */
#define MPIR_TSP_sched_finalize           MPIR_COLL_GENERIC_sched_finalize
/* Indicate that schedule generation is now complete */
#define MPIR_TSP_sched_commit             MPIR_COLL_GENERIC_sched_commit
