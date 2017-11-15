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

/* Define the transport functions */
#define TRANSPORT_NAME GENERIC_
#include "generic_tsp_api_def.h"
/* Instantiate algorithm functions */
#include "../algorithms/tree/namespace_def.h"

#include "bcast_tree.h"

/* Undef algorithm functions */
#include "../algorithms/tree/namespace_undef.h"
/* Undef transport functions */
#include "tsp_api_undef.h"
#undef TRANSPORT_NAME
