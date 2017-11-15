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
#include "uthash.h"
#include "mpir_func.h"
#include "coll_sched_db.h"

/* function to add schedule to the database */
void MPIR_COLL_add_sched(MPIR_COLL_sched_entry_t ** table, void *coll_args, int size, void *sched)
{
    MPIR_COLL_sched_entry_t *s = NULL;
    HASH_FIND(handle, *table, coll_args, size, s);
    if (s == NULL) {
        s = (MPIR_COLL_sched_entry_t *) MPL_calloc(sizeof(MPIR_COLL_sched_entry_t) + size, 1,
                                                   MPL_MEM_COLL);
        memcpy(s->arg, (char *) coll_args, size);
        s->size = size;
        s->sched = sched;
        HASH_ADD(handle, *table, arg, size, s, MPL_MEM_COLL);
    }
}

/* function to retrieve schedule from a database */
void *MPIR_COLL_get_sched(MPIR_COLL_sched_entry_t * table, void *coll_args, int size)
{
    MPIR_COLL_sched_entry_t *s = NULL;
    HASH_FIND(handle, table, coll_args, size, s);
    if (s)
        return s->sched;
    else
        return NULL;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_delete_sched_table
/* function to delete all schedule entries in a table */
void MPIR_COLL_delete_sched_table(MPIR_COLL_sched_entry_t * table, MPIR_COLL_sched_free_fn free_fn)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COLL_DELETE_SCHED_TABLE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COLL_DELETE_SCHED_TABLE);

    MPIR_COLL_sched_entry_t *current_sched, *tmp;
    current_sched = tmp = NULL;

    HASH_ITER(handle, table, current_sched, tmp) {
        HASH_DELETE(handle, table, current_sched);      /* delete; MPIR_COLL_sched_table advances to next */
        free_fn(current_sched->sched);  /* frees any memory associated with the schedule
                                         * and then the schedule itself */
        MPL_free(current_sched);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COLL_DELETE_SCHED_TABLE);
}
