/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "coll_tree_types.h"

#ifndef MPIR_COLL_TREE_IMPL_H
#define MPIR_COLL_TREE_IMPL_H

/* Initialize collective algorithm */
int MPIR_COLL_tree_init(void);

/* Initialize algorithm specific communicator */
int MPIR_COLL_tree_comm_init(MPIR_Comm * comm);

/* Initialize algorithm specific communicator data to NULL */
int MPIR_COLL_tree_comm_init_null(MPIR_Comm * comm);

/* Cleanup algorithm communicator */
int MPIR_COLL_tree_comm_cleanup(MPIR_Comm * comm);

/* Get size of key for tree algorithms */
int MPIR_COLL_tree_get_key_size(MPIR_COLL_tree_args_t * key);

#endif /* MPIR_COLL_TREE_IMPL_H */
