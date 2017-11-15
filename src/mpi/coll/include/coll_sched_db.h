/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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

#ifndef MPIR_COLL_SCHED_DB_H_INCLUDED
#define MPIR_COLL_SCHED_DB_H_INCLUDED

/* function to add schedule to the database */
void MPIR_COLL_add_sched(MPIR_COLL_sched_entry_t ** table, void *coll_args, int size, void *sched);

/* function to retrieve schedule from a database */
void *MPIR_COLL_get_sched(MPIR_COLL_sched_entry_t * table, void *coll_args, int size);

/* function to delete all schedule entries in a table */
void MPIR_COLL_delete_sched_table(MPIR_COLL_sched_entry_t * table, MPIR_COLL_sched_free_fn free_fn);

#endif /* MPIR_COLL_SCHED_DB_H_INCLUDED */
