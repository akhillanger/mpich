/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef NETMOD_OFI_TRIGGERED_TREE_PRE_H_INCLUDED
#define NETMOD_OFI_TRIGGERED_TREE_PRE_H_INCLUDED

#include "./coll_impl.h"

/* -------------------------------------------------------------------------- */
/* Collectives for the stub transport                                         */
/* -------------------------------------------------------------------------- */
#define GLOBAL_NAME    MPIDI_OFI_COLL_
#define TRANSPORT_NAME STUB_

#include "../transports/stub/transport_types.h"

/* Template: stub collectives */
#define COLL_NAME      STUB_
#include "templates/stub_pre.h"

/* Template:  binary trees */
#define COLL_NAME              KARY_
#include "templates/binary_tree_pre.h"

/* Template:  binomial tree */
#define COLL_NAME              KNOMIAL_
#include "templates/binomial_tree_pre.h"

/* Template:  Dissemination */
#define COLL_NAME              DISSEM_
#include "templates/dissemination_pre.h"

/*Template: Recursive Exchange */
#define COLL_NAME              RECEXCH_
#include "templates/recexch_pre.h"

#include "tsp_namespace_post.h"

/* -------------------------------------------------------------------------- */
/* Collectives for the triggered transport                                    */
/* -------------------------------------------------------------------------- */
//#define GLOBAL_NAME    MPIDI_OFI_COLL_
//#define TRANSPORT_NAME TRIGGERED_
//#include "../transports/triggered/transport_types.h"
//
//#define COLL_NAME      STUB_
//#include "templates/stub_pre.h"
//
///* Template:  binary trees */
//#define COLL_NAME              KARY_
//#include "templates/binary_tree_pre.h"
//
///* Template:  binomial tree */
//#define COLL_NAME              KNOMIAL_
//#include "templates/binomial_tree_pre.h"
//
///* Template:  Dissemination */
//#define COLL_NAME              DISSEM_
//#include "templates/dissemination_pre.h"
//
///* Template:  Recursive Exchange */
//#define COLL_NAME              RECEXCH_
//#include "templates/recexch_pre.h"
//
//#include "tsp_namespace_post.h"

/* -------------------------------------------------------------------------- */
/* Collectives for the triggered transport                                    */
/* -------------------------------------------------------------------------- */
#define GLOBAL_NAME    MPIDI_OFI_COLL_
#define TRANSPORT_NAME MPICH_
#include "../transports/mpich/transport_types.h"

#define COLL_NAME      STUB_
#include "templates/stub_pre.h"

/* Template:  binary trees */
#define COLL_NAME              KARY_
#include "templates/binary_tree_pre.h"

/* Template:  binomial tree */
#define COLL_NAME              KNOMIAL_
#include "templates/binomial_tree_pre.h"

/* Template:  Dissemination */
#define COLL_NAME              DISSEM_
#include "templates/dissemination_pre.h"

/* Template:  Recursive Exchange */
#define COLL_NAME              RECEXCH_
#include "templates/recexch_pre.h"

#include "tsp_namespace_post.h"

/* -------------------------------------------------------------------------- */
/* Collectives for the triggered transport                                    */
/* Note:  Does not depend on transport                                        */
/* -------------------------------------------------------------------------- */
#define GLOBAL_NAME    MPIDI_OFI_COLL_
#define TRANSPORT_NAME SHM_
#define COLL_NAME      GR_
#include "templates/shm_gr_pre.h"
#undef  TRANSPORT_NAME
#undef  GLOBAL_NAME

#endif