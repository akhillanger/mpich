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
#include "generic_tsp_types.h"
#include "generic_tsp.h"
#include "utarray.h"

#ifndef GENERIC_TSP_UTILS_H
#define GENERIC_TSP_UTILS_H

void MPII_COLL_GENERIC_vtx_t_copy(void *_dst, const void *_src);
void MPII_COLL_GENERIC_vtx_t_dtor(void *_elt);
void MPII_COLL_GENERIC_issue_vtx(int, MPIR_COLL_GENERIC_vtx_t *, MPIR_COLL_GENERIC_sched_t *);
void MPII_COLL_GENERIC_decrement_num_unfinished_dependecies(MPIR_COLL_GENERIC_vtx_t * vtxp,
                                                            MPIR_COLL_GENERIC_sched_t * sched);
void MPII_COLL_GENERIC_record_vtx_completion(MPIR_COLL_GENERIC_vtx_t * vtxp,
                                             MPIR_COLL_GENERIC_sched_t * sched);
void MPII_COLL_GENERIC_record_vtx_issue(MPIR_COLL_GENERIC_vtx_t * vtxp,
                                        MPIR_COLL_GENERIC_sched_t * sched);
void MPII_COLL_GENERIC_reset_issued_list(MPIR_COLL_GENERIC_sched_t * sched);
void MPII_COLL_GENERIC_allocate_vtx(MPIR_COLL_GENERIC_vtx_t * vtx);
void MPII_COLL_GENERIC_init_vtx(MPIR_COLL_GENERIC_sched_t * sched, MPIR_COLL_GENERIC_vtx_t * vtx,
                                int id);
void MPII_COLL_GENERIC_free_vtx(MPIR_COLL_GENERIC_vtx_t * vtx);
void MPII_COLL_GENERIC_extend_int_utarray(UT_array * in, int n_elems, int *elems);
void MPII_COLL_GENERIC_add_vtx_dependencies(MPIR_COLL_GENERIC_sched_t * sched, int vtx_id,
                                            int n_invtcs, int *invtcs);
int MPII_COLL_GENERIC_get_new_vtx(MPIR_COLL_GENERIC_sched_t * sched,
                                  MPIR_COLL_GENERIC_vtx_t ** vtx);
int MPII_COLL_GENERIC_do_reduce_in_recv_reduce(void *data, MPI_Status * status);
void MPII_COLL_GENERIC_add_elem_ptr_array(UT_array * buf_array, void *buf);
void MPII_COLL_GENERIC_issue_vtx(int vtxid, MPIR_COLL_GENERIC_vtx_t * vtxp,
                                 MPIR_COLL_GENERIC_sched_t * sched);
void MPII_COLL_GENERIC_free_buffers(MPIR_COLL_GENERIC_sched_t * sched);
void MPII_COLL_GENERIC_sched_destroy_fn(MPIR_COLL_GENERIC_sched_t * sched);

#endif /* GENERIC_TSP_UTILS_H */
