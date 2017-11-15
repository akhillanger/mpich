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

#include "coll_tree_types.h"
#include "coll_tree_util.h"
#include "coll_util.h"
#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_init_tree
/* Generate the tree */
int MPIR_COLL_init_tree(int rank, int nranks, int tree_type, int k, int root, MPIR_COLL_tree_t * ct)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COLL_INIT_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COLL_INIT_TREE);

    int mpi_errno = MPI_SUCCESS;
    ct->children = NULL;
    ct->max_children = 0;

    if (tree_type == TREE_TYPE_KNOMIAL) /* knomial tree */
        MPIR_COLL_init_knomial_tree(rank, nranks, k, root, ct);
    else if (tree_type == TREE_TYPE_KARY)       /* kary tree */
        MPIR_COLL_init_kary_tree(rank, nranks, k, root, ct);
    else {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "tree_type %d not defined, initializing knomial tree\n",
                         tree_type));
        MPIR_COLL_init_knomial_tree(rank, nranks, k, root, ct);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COLL_INIT_TREE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_free_tree
/* Free any memory associate with MPIR_COLL_tree_t */
void MPIR_COLL_free_tree(MPIR_COLL_tree_t * tree)
{
    MPL_free(tree->children);
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_dump_tree
/* Utility routine to print a tree */
int MPIR_COLL_dump_tree(int tree_size, int root, int tree_type, int k)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COLL_DUMP_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COLL_DUMP_TREE);

    int mpi_errno = MPI_SUCCESS;
    int i;

    char str_out[1000];
    int str_len = 0;
    for (i = 0; i < tree_size; i++) {
        MPIR_COLL_tree_t ct;
        int j;

        mpi_errno = MPIR_COLL_init_tree(i, tree_size, tree_type, k, root, &ct);
        MPIR_Assert(str_len <= 950);    /* make sure there is enough space to write buffer */
        str_len += sprintf(str_out + str_len, "%d -> %d, ", ct.rank, ct.parent);

        MPIR_Assert(str_len <= 950);
        str_len += sprintf(str_out + str_len, "{");
        for (j = 0; j < ct.num_children; j++) {
            int tmp = ct.children[j];
            MPIR_Assert(str_len <= 950);
            str_len += sprintf(str_out + str_len, "%d, ", tmp);
        }
        MPIR_Assert(str_len <= 950);
        str_len += sprintf(str_out + str_len, "}\n");

        MPIR_COLL_free_tree(&ct);
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, str_out));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COLL_DUMP_TREE);

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIR_COLL_add_child_tree
/* utility function to add a child to MPIR_COLL_tree_t data structure */
void MPIR_COLL_add_child_tree(MPIR_COLL_tree_t * t, int rank)
{
    if (t->num_children + 1 > t->max_children) {
        int *old_children = t->children;
        t->children = (int *) MPL_malloc(sizeof(int) * 2 * t->max_children, MPL_MEM_COLL);
        t->max_children = 2 * t->max_children;
        memcpy(t->children, old_children, sizeof(int) * t->max_children);
        MPL_free(old_children);
    }
    t->children[t->num_children++] = rank;
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_init_kary_tree
/* Function to generate kary tree information for rank 'rank' and store it in MPIR_COLL_tree_t data structure */
void MPIR_COLL_init_kary_tree(int rank, int nranks, int k, int root, MPIR_COLL_tree_t * ct)
{
    int lrank, child;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->num_children = 0;
    ct->parent = -1;
    ct->children = (int *) MPL_malloc(sizeof(int) * k, MPL_MEM_COLL);
    ct->max_children = k;

    if (nranks <= 0)
        return;

    lrank = (rank + (nranks - root)) % nranks;
    MPIR_Assert(k >= 2);

    ct->parent = (lrank <= 0) ? -1 : (((lrank - 1) / k) + root) % nranks;

    for (child = 1; child <= k; child++) {
        int val = lrank * k + child;
        if (val >= nranks)
            break;
        val = (val + root) % nranks;
        MPIR_COLL_add_child_tree(ct, val);
    }
}

#undef FUNCNAME
#define FUNCNAME MPIR_COLL_init_knomial_tree
/* Function to generate knomial tree information for rank 'rank' and store it in MPIR_COLL_tree_t data structure */
void MPIR_COLL_init_knomial_tree(int rank, int nranks, int k, int root, MPIR_COLL_tree_t * ct)
{
    int lrank, i, j, maxtime, tmp, time, parent, current_rank, running_rank, crank;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->num_children = 0;
    ct->parent = -1;

    if (nranks <= 0)
        return;

    lrank = (rank + (nranks - root)) % nranks;
    MPIR_Assert(k >= 2);

    maxtime = 0;        /* maximum number of steps while generating the knomial tree */
    tmp = nranks - 1;
    while (tmp) {
        maxtime++;
        tmp /= k;
    }
    /* initialize to NULL first, so that there are no issues during free'ing */
    ct->children = NULL;
    ct->max_children = 0;

    if (maxtime)
        ct->children = (int *) MPL_malloc(sizeof(int) * (maxtime) * (k - 1), MPL_MEM_COLL);
    ct->max_children = maxtime * (k - 1);
    time = 0;
    parent = -1;        /* root has no parent */
    current_rank = 0;   /* start at root of the tree */
    running_rank = current_rank + 1;    /* used for calculation below
                                         * start with first child of the current_rank */

    while (true) {
        if (lrank == current_rank)      /* desired rank found */
            break;
        for (j = 1; j < k; j++) {
            if (lrank >= running_rank && lrank < running_rank + MPIU_ipow(k, maxtime - time - 1)) {     /* check if rank lies in this range */
                /* move to the corresponding subtree */
                parent = current_rank;
                current_rank = running_rank;
                running_rank = current_rank + 1;
                break;
            } else
                running_rank += MPIU_ipow(k, maxtime - time - 1);
        }
        time++;
        MPIR_Assert(time <= nranks);    /* actually, should not need more steps than log_k(nranks) */
    }

    if (parent == -1)
        ct->parent = -1;
    else
        ct->parent = (parent + root) % nranks;

    crank = lrank + 1;  /* cranks stands for child rank */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "parent of rank %d is %d, total ranks = %d (root=%d)\n", rank,
                     ct->parent, nranks, root));
    for (i = time; i < maxtime; i++) {
        for (j = 1; j < k; j++) {
            if (crank < nranks) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "adding child %d to rank %d\n",
                                 (crank + root) % nranks, rank));
                MPIR_COLL_add_child_tree(ct, (crank + root) % nranks);
            }
            crank += MPIU_ipow(k, maxtime - i - 1);
        }
    }
}
