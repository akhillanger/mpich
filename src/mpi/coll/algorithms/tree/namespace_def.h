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

#ifndef MPIR_TSP_CAT_GUARD
#define MPIR_TSP_CAT_GUARD

#define MPIR_TSP_NSCAT0(a,b) a##b
#define MPIR_TSP_NSCAT1(a,b) MPIR_TSP_NSCAT0(a,b)

#endif /* MPIR_TSP_CAT_GUARD */

#ifndef MPIR_TSP_NAMESPACE
#define MPIR_TSP_NAMESPACE(fn)  MPIR_TSP_NSCAT1(MPIR_TSP_NSCAT1(MPIR_COLL_, TRANSPORT_NAME), fn)
#endif /* MPIR_TSP_NAMESPACE */

/* Algorithm API */
#define MPIR_TSP_tree_bcast_nb              MPIR_TSP_NAMESPACE(tree_bcast_nb)
#define MPIR_TSP_tree_ibcast                MPIR_TSP_NAMESPACE(tree_ibcast)

/* Tree scheduling utility functions */
#define MPIR_TSP_get_bcast_tree_schedule    MPIR_TSP_NAMESPACE(bcast_get_tree_schedule)
#define MPIR_TSP_sched_bcast_tree           MPIR_TSP_NAMESPACE(sched_bcast_tree)
#define MPIR_TSP_sched_bcast_tree_pipelined MPIR_TSP_NAMESPACE(sched_bcast_tree_pipelined)
