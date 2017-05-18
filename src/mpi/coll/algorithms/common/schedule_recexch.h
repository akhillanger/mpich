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

#include "recexch_util.h"

#ifndef COLL_NAMESPACE
#error "The tree template must be namespaced with COLL_NAMESPACE"
#endif


/*This function calculates the ranks to/from which the
data is sent/recvd in various steps/phases of recursive koupling
algorithm. Recursive Koupling is divided into three steps - Step 1, 2 and 3.
Step 2 is the main step that does the recursive koupling.
Step 1 and Step 3 are done to handle the case of non-power-of-k number of ranks.
Only p_of_k (largest power of k that is less than n) ranks participate in Step 2.
In Step 1, the non partcipating ranks send their data to ranks
participating in Step 2. In Step 3, the ranks participating in Step 2 send
the final data to non-partcipating ranks.

Step 2 is further divided into log_k(p_of_k) phases.

Ideally, this function belongs to recexch_util.h file but because this
function uses TSP_allocate_mem, it had to be put here
*/
static inline
    int COLL_get_neighbors_recexch(int rank, int nranks, int *k_, int *step1_sendto,
                                   int **step1_recvfrom_, int *step1_nrecvs,
                                   int ***step2_nbrs_, int *step2_nphases, int *p_of_k_, int *T_)
{
    int i, j, k;
    k = *k_;
    if (nranks < k)     /*If size of the communicator is less than k, reduce the value of k */
        k = (nranks > 2) ? nranks : 2;
    *k_ = k;
    /*p_of_k is the largest power of k that is less than nranks */
    int p_of_k = 1, log_p_of_k = 0, rem, T, newrank;

    while (p_of_k <= nranks) {
        p_of_k *= k;
        log_p_of_k++;
    }

    p_of_k /= k;
    log_p_of_k--;

    MPIC_DBG( "allocate memory for storing communication pattern\n");

    *step2_nbrs_ = (int **) TSP_allocate_mem(sizeof(int *) * log_p_of_k);

    for (i = 0; i < log_p_of_k; i++) {
        (*step2_nbrs_)[i] = (int *) TSP_allocate_mem(sizeof(int) * (k - 1));
    }

    int **step2_nbrs = *step2_nbrs_;
    int *step1_recvfrom = *step1_recvfrom_ = (int *) TSP_allocate_mem(sizeof(int) * (k - 1));
    *step2_nphases = log_p_of_k;

    rem = nranks - p_of_k; 
    /*rem is the number of ranks that do not particpate in Step 2 
     * We need to identify the non-participating ranks.
     * The first T ranks are divided into sets of k consecutive ranks each.
     * From each of these sets, the first k-1 ranks are
     * the non-participatig ranks while the last rank participates in Step 2.
     * The non-participating ranks send their data to the participating rank
     * in their set.
     */
    T = (rem * k) / (k - 1);
    *T_ = T;
    *p_of_k_ = p_of_k;


    MPIC_DBG( "step 1 nbr calculation started. T is %d \n", T);
    *step1_nrecvs = 0;
    *step1_sendto = -1;

    /* Step 1 */
    if (rank < T) {
        if (rank % k != (k - 1)) {
            /* I am a non-participating rank */
            *step1_sendto = rank + (k - 1 - rank % k);
            /* partipating rank to send the data to */

            if (*step1_sendto > T - 1)
                *step1_sendto = T;
            /*if the corresponding participating rank is not in T,
             *
             * then send to the Tth rank to preserve non-commutativity */
            newrank = -1;
            /*tag this rank as non-participating */
        }
        else {
            /* receiver rank */
            for (i = 0; i < k - 1; i++) {
                step1_recvfrom[i] = rank - i - 1;
            }

            *step1_nrecvs = k - 1;
            /*this is the new rank in the set of participating ranks */
            newrank = rank / k;
        }
    }
    else {
        /* rank >= T */
        newrank = rank - rem;

        if (rank == T && (T - 1) % k != k - 1 && T >= 1) {
            int nsenders = (T - 1) % k + 1;
            /*number of ranks sending to me */

            for (j = nsenders - 1; j >= 0; j--) {
                step1_recvfrom[nsenders - 1 - j] = T - nsenders + j;
            }

            *step1_nrecvs = nsenders;
        }
    }

    MPIC_DBG( "step 1 nbr computation completed\n");

    /*Step 2 */
    if (*step1_sendto == -1) {
        /* calulate step2_nbrs only for participating ranks */
        int *digit = (int *) TSP_allocate_mem(sizeof(int) * log_p_of_k);
        /*calculate the digits in base k representation of newrank */
        for (i = 0; i < log_p_of_k; i++)
            digit[i] = 0;
        int temprank = newrank, index = 0, remainder;
        while (temprank != 0) {
            remainder = temprank % k;
            temprank = temprank / k;
            digit[index] = remainder;
            index++;
        }

        int mask = 0x1;
        int phase = 0, cbit, cnt, nbr, power;
        while (mask < p_of_k) {
            cbit = digit[phase];
            /*phase_th digit changes in this phase, obtain its original value */
            cnt = 0;
            for (i = 0; i < k; i++) {
                /*there are k-1 neighbors */
                if (i != cbit) {
                    /*do not generate yourself as your nieighbor */
                    digit[phase] = i;
                    /*this gets us the base k representation of the neighbor */

                    /*calculate the base 10 value of the neighbor rank */
                    nbr = 0;
                    power = 1;
                    for (j = 0; j < log_p_of_k; j++) {
                        nbr += digit[j] * power;
                        power *= k;
                    }

                    /*calculate its real rank and store it */
                    step2_nbrs[phase][cnt] =
                        (nbr < rem / (k - 1)) ? (nbr * k) + (k - 1) : nbr + rem;
                    MPIC_DBG("step2_nbrs[%d][%d] is %d \n",
                            phase, cnt, step2_nbrs[phase][cnt]);
                    cnt++;
                }
            }
            MPIC_DBG("step 2, phase %d nbr calculation completed\n", phase);
            digit[phase] = cbit;
            phase++;
            mask *= k;
        }

        TSP_free_mem(digit);
    }

    return 0;
}

static inline int
COLL_sched_allreduce_recexch(const void *sendbuf,
                             void *recvbuf,
                             int count,
                             COLL_dt_t datatype,
                             COLL_op_t op,
                             int tag,
                             COLL_comm_t * comm,
                             int k, TSP_sched_t * s, int per_nbr_buffer, int finalize)
{

    int is_inplace, is_contig, i, j;
    int dtcopy_id = -1;
    TSP_sched_t *sched = s;
    TSP_comm_t *tsp_comm = &comm->tsp_comm;

    size_t type_size, extent, lb;
    int is_commutative;
    TSP_opinfo(op, &is_commutative);
    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);

    is_inplace = TSP_isinplace((void *) sendbuf);
    int nranks = TSP_size(tsp_comm);
    int rank = TSP_rank(tsp_comm);
    /*if there is only 1 rank, copy data from sendbuf
     * to recvbuf and exit */
    if (nranks == 1) {
        if (!is_inplace && count > 0)
            TSP_dtcopy_nb(recvbuf, count, datatype, sendbuf,
                            count, datatype, sched, 0, NULL);
        return 0;
    }

    int step1_sendto = -1;
    int step2_nphases;
    int step1_nrecvs;

    /*COLL_get_neighbors_recexch function allocates memory
     * to these pointers */
    int *step1_recvfrom;
    int **step2_nbrs;
    int p_of_k, T;
    MPIC_DBG( "Beforeinitial dt copy\n");
    /*get the neighbors */
    COLL_get_neighbors_recexch(rank, nranks, &k, &step1_sendto,
                                &step1_recvfrom, &step1_nrecvs,
                                &step2_nbrs, &step2_nphases, &p_of_k, &T);
    bool in_step2 = (step1_sendto == -1);

    void *tmp_buf = TSP_allocate_buffer(count * extent, sched);

    if (in_step2 && !is_inplace && count > 0) {
        /*copy the data to recvbuf but only if you are a rank participating in Step 2 */
        dtcopy_id = TSP_dtcopy_nb(recvbuf, count, datatype,
                                  sendbuf, count, datatype, sched, 0, NULL);
    }

    MPIC_DBG( "After initial dt copy\n");
    int *send_id = (int *) TSP_allocate_mem(sizeof(int) * k);   /*to store send ids */
    int *reduce_id = (int *) TSP_allocate_mem(sizeof(int) * k); /*to store recv_reduce ids */
    /*Step 1 */
    if (!in_step2) {
        /*non-participating rank sends the data to a partcipating rank */
        void *buf_to_send;
        if (is_inplace)
            buf_to_send = recvbuf;
        else
            buf_to_send = sendbuf;
        TSP_send(buf_to_send, count, datatype, step1_sendto, tag,
                        tsp_comm, sched, 0, NULL);
    }

    void **step1_recvbuf = (void **) TSP_allocate_mem(
                                    sizeof(void *) * step1_nrecvs);
    if (per_nbr_buffer != 1 && step1_nrecvs > 0)
        step1_recvbuf[0] = TSP_allocate_buffer(count * extent, sched);
    int buf, recv_id;
    int nvtcs;
    for (i = 0; i < step1_nrecvs; i++) {
        /*participating rank gets data from non-partcipating ranks */
        buf = (per_nbr_buffer == 1) ? i : 0;
        int vtcs[2];
        if (per_nbr_buffer == 1)
            step1_recvbuf[buf] = TSP_allocate_buffer(count * extent, sched);
        /* recv dependencies */
        if (i == 0 || per_nbr_buffer == 1 || count == 0)
            nvtcs = 0;
        else {
            nvtcs = 1;
            vtcs[0] = reduce_id[i - 1];
        }
        recv_id = TSP_recv(step1_recvbuf[buf], count, datatype,
                           step1_recvfrom[i], tag, tsp_comm,
                           sched, nvtcs, vtcs);
        if (count != 0) {/* Reduce only if data is present */
            /* reduce dependencies */
            if (is_commutative) {
                if (!is_inplace) {
                    /*wait for the data to be copied to recvbuf */
                    nvtcs = 2;
                    vtcs[0] = dtcopy_id;
                    vtcs[1] = recv_id;
                }
                else {
                    /*i have no dependency */
                    nvtcs = 1;
                    vtcs[0] = recv_id;
                }
            }
            else {
                //if not commutative
                if (i == 0 && is_inplace) {
                    /*if data is inplace, no need to wait */
                    nvtcs = 1;
                    vtcs[0] = recv_id;
                }
                else {
                    /*wait for datacopy to complete if i==0 else wait for previous recv_reduce */
                    nvtcs = 2;
                    if (i == 0)
                        vtcs[0] = dtcopy_id;
                    else
                        vtcs[0] = reduce_id[i - 1];
                    vtcs[1] = recv_id;
                }
            }
            reduce_id[i] = TSP_reduce_local(step1_recvbuf[buf], recvbuf, count,
                                            datatype, op, TSP_FLAG_REDUCE_L,
                                            sched, nvtcs, vtcs);
        }
    }
    int fence_id = TSP_fence(sched);

    /*Step 2 */
    int myidx, nbr, phase;
    int *vtcs = TSP_allocate_mem(sizeof(int) * 2 * k);
    /*used for specifying graph dependencies */
    MPIC_DBG( "After step1");

    void **nbr_buffer = (void **) TSP_allocate_mem(
            sizeof(void *) * step2_nphases * (k - 1));
    buf = 0;
    if (step2_nphases > 0)
        nbr_buffer[0] = TSP_allocate_buffer(count * extent, sched);
    for (i = 0; i < step2_nphases; i++) {
        for (j = 0; j < (k - 1); j++, buf++) {
            if (buf == 0)
                /*memory is already allocated for the first child above */
                continue;
            else {
                if (per_nbr_buffer) {
                    nbr_buffer[buf] = TSP_allocate_buffer(
                                                count * extent, sched);
                }
                else
                    nbr_buffer[buf] = nbr_buffer[0];
            }
        }
    }
    buf = 0;
    for (phase = 0; phase < step2_nphases && step1_sendto == -1; phase++) {
        if (!is_commutative) {
            /*sort the neighbor list so that receives can be posted in order */
            qsort(step2_nbrs[phase], k - 1, sizeof(int), intcmpfn);
        }
        /*copy the data to a temporary buffer so that sends ands recvs
         * can be posted simultaneosly */
        if (count != 0) {
            if (phase == 0) {
                /*wait for Step 1 to complete */
                nvtcs = 1;
                vtcs[0] = fence_id;
            }
            else {
                nvtcs = k - 1;
                for (i = 0; i < k - 1; i++)
                    vtcs[i] = send_id[i];
                if (is_commutative) {
                    /*wait for all the previous recv_reduce to complete */
                    for (i = 0; i < k - 1; i++)
                        vtcs[i + nvtcs] = reduce_id[i];
                    nvtcs += k - 1;
                }
                else {
                    /*just wait for the last recv_reduce to complete as recv_reduce happen in order */
                    vtcs[nvtcs] = reduce_id[k - 2];
                    nvtcs += 1;
                    /*wait for the previous sends to complete and the last recv_reduce */
                }
            }
            dtcopy_id = TSP_dtcopy_nb(tmp_buf, count, datatype,
                                      recvbuf, count, datatype,
                                      sched, nvtcs, vtcs);
            MPIC_DBG("Step 1: data copy scheduled\n");
        }
        /*myidx is the index in the neighbors list such that
         * all neighbors before myidx have ranks less than my rank
         * and neighbors at and after myidx have rank greater than me.
         * This is useful only in the non-commutative case. */
        myidx = 0;
        /*send data to all the neighbors */
        for (i = 0; i < k - 1; i++) {
            if (count == 0)
                nvtcs = 0;
            else {
                nvtcs = 1;
                vtcs[0] = dtcopy_id;
            }
            nbr = step2_nbrs[phase][i];
            send_id[i] = TSP_send( tmp_buf, count, datatype, nbr, tag,
                                   tsp_comm, sched, nvtcs, vtcs);
            if (rank > nbr) {
                myidx = i + 1;
            }
        }
        int counter = 0;
        /*receive from the neighbors to the left. Need datacopy here */
        for (i = myidx - 1; i >= 0; i--, buf++) {
            nbr = step2_nbrs[phase][i];
            if (per_nbr_buffer == 1 || counter == 0 || count == 0)
                nvtcs = 0;
            else {
                nvtcs = 1;
                vtcs[0] = reduce_id[counter - 1];
            }
            recv_id = TSP_recv(nbr_buffer[buf], count, datatype, nbr,
                               tag, tsp_comm, sched, nvtcs, vtcs);

            if (count != 0) {
                nvtcs = 2;
                if (counter == 0)
                    vtcs[0] = dtcopy_id;
                else
                    vtcs[0] = (is_commutative) ? dtcopy_id : reduce_id[counter - 1];
                vtcs[1] = recv_id;
                reduce_id[counter++] = TSP_reduce_local(nbr_buffer[buf],
                                                        recvbuf, count,
                                                        datatype, op,
                                                        TSP_FLAG_REDUCE_L, sched,
                                                        nvtcs, vtcs);
            }

        }

        /*receive from the neighbors to the right */
        for (i = myidx; i < k - 1; i++, buf++) {
            nbr = step2_nbrs[phase][i];
            if (per_nbr_buffer == 1 || counter == 0 || count == 0)
                nvtcs = 0;
            else {
                nvtcs = 1;
                vtcs[0] = reduce_id[counter - 1];
            }
            recv_id = TSP_recv(nbr_buffer[buf], count, datatype, nbr,
                               tag, tsp_comm, sched, nvtcs, vtcs);

            if (count != 0) {
                nvtcs = 2;
                if (counter == 0)
                    vtcs[0] = dtcopy_id;
                else
                    vtcs[0] = (is_commutative) ? dtcopy_id : reduce_id[counter - 1];
                vtcs[1] = recv_id;
                reduce_id[counter++] = TSP_reduce_local(nbr_buffer[buf], recvbuf, count,
                                                        datatype, op, TSP_FLAG_REDUCE_R, sched,
                                                        nvtcs, vtcs);
            }
        }
    }
    MPIC_DBG("After step2\n");

    /*Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (step1_sendto != -1) {
        TSP_recv(recvbuf, count, datatype, step1_sendto, tag,
                    tsp_comm, sched, 0, NULL);
        //TSP_free_mem(tmp_buf);
    }
    else {      /* free temporary buffer after it is used in last send */
        //TSP_free_mem_nb(tmp_buf,sched,k-1,id+1);
    }

    for (i = 0; i < step1_nrecvs; i++) {
        if (is_commutative) {
            /* If commutative, wait for all the prev reduce calls to complete
             * since they can happen in any order */
            nvtcs = k - 1;
            for (i = 0; i < k - 1; i++)
                vtcs[i] = reduce_id[i];
        }
        else {
            /* If non commutative, wait for the last reduce to complete, as they happen in order */
            nvtcs = 1;
            vtcs[0] = reduce_id[k - 2];
        }
        TSP_send(recvbuf, count, datatype, step1_recvfrom[i], tag, tsp_comm, sched, nvtcs, vtcs);
    }

    MPIC_DBG( "After step3\n");
    /*free all allocated memory for storing nbrs */
    for (i = 0; i < step2_nphases; i++)
        TSP_free_mem(step2_nbrs[i]);
    TSP_free_mem(step2_nbrs);
    MPIC_DBG("freed step2_nbrs\n");
    TSP_free_mem(step1_recvfrom);

    MPIC_DBG("freed step1_recvfrom\n");

    TSP_free_mem(send_id);
    TSP_free_mem(reduce_id);
    TSP_free_mem(vtcs);

    if (finalize) {
        TSP_sched_commit(sched);
    }

    return 0;
}

#if 0
static inline int
COLL_sched_allreduce_recexch(const void *sendbuf,
                             void *recvbuf,
                             int count,
                             COLL_dt_t datatype,
                             COLL_op_t op,
                             int tag, COLL_comm_t * comm, COLL_sched_t * s, int finalize)
{

    int k = MPIR_CVAR_ALLRED_RECEXCH_KVAL;
    int is_inplace, is_contig, i;
    int dtcopy_id = -1;
    int nranks = TSP_size(&comm->tsp_comm);
    int rank = TSP_rank(&comm->tsp_comm);

    size_t type_size, extent, lb;
    int is_commutative;
    TSP_opinfo(op, &is_commutative);
    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);

    is_inplace = TSP_isinplace((void *) sendbuf);
    /*if there is only 1 rank, copy data from sendbuf
     * to recvbuf and exit */
    if (nranks == 1) {
        if (!is_inplace)
            TSP_dtcopy_nb(recvbuf, count, datatype,
                          sendbuf, count, datatype, &s->tsp_sched, 0, NULL);
        return 0;
    }

    int step1_sendto = -1;
    int step2_nphases;
    int step1_nrecvs;

    /*COLL_get_neighbors_recexch function allocates memory
     * to these pointers */
    int *step1_recvfrom;
    int **step2_nbrs;
    int p_of_k, T;
    /*get the neighbors */
    COLL_get_neighbors_recexch(rank, nranks, &k, &step1_sendto, &step1_recvfrom, &step1_nrecvs,
                               &step2_nbrs, &step2_nphases, &p_of_k, &T);

    void *tmp_buf = TSP_allocate_buffer(count * extent, &s->tsp_sched);

    if (!is_inplace && step1_sendto == -1) {    /*copy the data to recvbuf but only if you are a rank participating in Step 2 */
        dtcopy_id = TSP_dtcopy_nb(recvbuf, count, datatype,
                                  sendbuf, count, datatype, &s->tsp_sched, 0, NULL);
    }

    if (0)
        fprintf(stderr, "After initial dt copy\n");
    int *sid = (int *) TSP_allocate_mem(sizeof(int) * k);       /*to store send ids */
    int *rid = (int *) TSP_allocate_mem(sizeof(int) * k);       /*to store recv_reduce ids */
    /*Step 1 */
    if (step1_sendto != -1) {   /*non-participating rank sends the data to a partcipating rank */
        if (!is_inplace)
            TSP_send(sendbuf, count, datatype, step1_sendto,
                     tag, &comm->tsp_comm, &s->tsp_sched, 0, NULL);
        else
            TSP_send(recvbuf, count, datatype, step1_sendto,
                     tag, &comm->tsp_comm, &s->tsp_sched, 0, NULL);
    }

    for (i = 0; i < step1_nrecvs; i++) {        /*participating rank gets data from non-partcipating ranks */
        int nvtcs;
        int vtx;
        if (is_commutative) {
            if (!is_inplace) {  /*wait for the data to be copied to recvbuf */
                nvtcs = 1;
                vtx = dtcopy_id;
            }
            else {      /*i have no dependency */
                nvtcs = 0;
            }
        }
        else {  //if not commutative
            if (i == 0 && is_inplace) { /*if data is inplace, no need to wait */
                nvtcs = 0;
            }
            else {      /*wait for datacopy to complete if i==0 else wait for previous recv_reduce */
                nvtcs = 1;
                if (i == 0)
                    vtx = dtcopy_id;
                else
                    vtx = rid[i - 1];
            }
        }
        rid[i] = TSP_recv_reduce(recvbuf, count, datatype,
                                 op, step1_recvfrom[i], tag, &comm->tsp_comm,
                                 TSP_FLAG_REDUCE_L, &s->tsp_sched, nvtcs, &vtx);
    }
    int fence_id = TSP_fence(&s->tsp_sched);

    /*Step 2 */
    int myidx, nbr, phase;
    int nvtcs;
    int *vtcs = TSP_allocate_mem(sizeof(int) * 2 * k);  /*used for specifying graph dependencies */
    if (0)
        fprintf(stderr, "After step1\n");
    for (phase = 0; phase < step2_nphases && step1_sendto == -1; phase++) {
        if (!is_commutative) {  /*sort the neighbor list so that receives can be posted in order */
            qsort(step2_nbrs[phase], k - 1, sizeof(int), intcmpfn);
        }

        /*copy the data to a temporary buffer so that sends ands recvs
         * can be posted simultaneosly */
        if (phase == 0) {       /*wait for Step 1 to complete */
            nvtcs = 1;
            vtcs[0] = fence_id;
        }
        else {
            /*wait for all the previous sends to complete */
            nvtcs = k - 1;
            for (i = 0; i < k - 1; i++)
                vtcs[i] = sid[i];

            if (is_commutative) {       /*wait for all the previous recv_reduce to complete */
                for (i = 0; i < k - 1; i++)
                    vtcs[i + nvtcs] = rid[i];
                nvtcs += k - 1;
            }
            else {      /*just wait for the last recv_reduce to complete as recv_reduce happen in order */
                vtcs[nvtcs] = rid[k - 2];
                nvtcs += 1;     /*wait for the previous sends to complete and the last recv_reduce */
            }
        }
        dtcopy_id = TSP_dtcopy_nb(tmp_buf, count, datatype,
                                  recvbuf, count, datatype, &s->tsp_sched, nvtcs, vtcs);
        if (0)
            fprintf(stderr, "Step 1: data copy scheduled\n");
        /*myidx is the index in the neighbors list such that
         * all neighbors before myidx have ranks less than my rank
         * and neighbors at and after myidx have rank greater than me.
         * This is useful only in the non-commutative case. */
        myidx = 0;
        /*send data to all the neighbors */
        for (i = 0; i < k - 1; i++) {
            nbr = step2_nbrs[phase][i];
            sid[i] = TSP_send(tmp_buf, count, datatype,
                              nbr, tag, &comm->tsp_comm, &s->tsp_sched, 1, &dtcopy_id);
            if (rank > nbr) {
                myidx = i + 1;
            }
        }
        int counter = 0;
        /*receive from the neighbors to the left */
        for (i = myidx - 1; i >= 0; i--) {
            nvtcs = 1;
            if (counter == 0)
                *vtcs = dtcopy_id;
            else
                *vtcs = (is_commutative) ? dtcopy_id : rid[counter - 1];

            nbr = step2_nbrs[phase][i];
            rid[counter++] = TSP_recv_reduce(recvbuf, count, datatype, op, nbr,
                                             tag, &comm->tsp_comm, TSP_FLAG_REDUCE_L, &s->tsp_sched,
                                             nvtcs, vtcs);
        }

        /*receive from the neighbors to the right */
        for (i = myidx; i < k - 1; i++) {
            nvtcs = 1;
            if (counter == 0)
                *vtcs = dtcopy_id;
            else
                *vtcs = (is_commutative) ? dtcopy_id : rid[counter - 1];

            nbr = step2_nbrs[phase][i];
            rid[counter++] = TSP_recv_reduce(recvbuf, count, datatype, op, nbr,
                                             tag, &comm->tsp_comm, TSP_FLAG_REDUCE_R, &s->tsp_sched,
                                             nvtcs, vtcs);
        }
    }
    if (0)
        fprintf(stderr, "After step2\n");

    /*Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (step1_sendto != -1) {
        TSP_recv(recvbuf, count, datatype, step1_sendto, tag, &comm->tsp_comm, &s->tsp_sched, 0,
                 NULL);
        //TSP_free_mem(tmp_buf);
    }
    else {      /* free temporary buffer after it is used in last send */
        //TSP_free_mem_nb(tmp_buf,&s->tsp_sched,k-1,id+1);
    }

    for (i = 0; i < step1_nrecvs; i++) {
        TSP_send(recvbuf, count, datatype, step1_recvfrom[i],
                 tag, &comm->tsp_comm, &s->tsp_sched, (is_commutative) ? k - 1 : 1,
                 (is_commutative) ? rid : &rid[k - 2]);
    }

    if (0)
        fprintf(stderr, "After step3\n");
    /*free all allocated memory for storing nbrs */
    for (i = 0; i < step2_nphases; i++)
        TSP_free_mem(step2_nbrs[i]);
    TSP_free_mem(step2_nbrs);
    if (0)
        fprintf(stderr, "freed step2_nbrs\n");
    TSP_free_mem(step1_recvfrom);

    if (0)
        fprintf(stderr, "freed step1_recvfrom\n");

    TSP_free_mem(sid);
    TSP_free_mem(rid);
    TSP_free_mem(vtcs);

    if (finalize) {
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}
#endif
