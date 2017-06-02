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

#include <math.h>

/*brucks Pack, UnPack (PUP) utility function
This functions packs (unpacks) non-contiguous (contiguous)
data from (to) rbuf to (from) pupbuf. It goes to every offset
that has the value "digitval" at the "phase"th digit in the
base k representation of the offset. The argument phase refers
to the phase of the brucks algorithm.
*/
static inline int
COLL_brucks_pup(bool pack, void *rbuf, void *pupbuf, COLL_dt_t rtype, int count,
                int phase, int k, int digitval, int comm_size, int *pupsize, TSP_sched_t * s,
                int ninvtcs, int *invtcs)
{
    size_t type_size, extent, lb;
    int is_contig;
    TSP_dtinfo(rtype, &is_contig, &type_size, &extent, &lb);

    int pow_k_phase = pow(k, phase);
    /*first offset where the phase'th bit has value digitval */
    int offset = pow_k_phase * digitval;
    /*number of consecutive occurences of digitval */
    int nconsecutive_occurrences = pow_k_phase;
    /*distance between non-consecutive occurences of digitval */
    int delta = (k - 1) * pow_k_phase;

    int *dtcopy_id = TSP_allocate_mem(sizeof(int) * comm_size);
    /**NOTE**: We do not need this large array - make it more accurate*/
    int counter = 0;
    *pupsize = 0;       /*points to the first empty location in pupbuf */
    while (offset < comm_size) {
        if (pack) {
            dtcopy_id[counter++] =
                TSP_dtcopy_nb(pupbuf + *pupsize, count, rtype, rbuf + offset * count * extent,
                              count, rtype, s, ninvtcs, invtcs);
            MPIC_DBG( "packing rbuf+%d to pupbuf+%d\n", offset * count * extent,
                        *pupsize);
        }
        else {
            dtcopy_id[counter++] =
                TSP_dtcopy_nb(rbuf + offset * count * extent, count, rtype, pupbuf + *pupsize,
                              count, rtype, s, ninvtcs, invtcs);
            MPIC_DBG( "unpacking from pupbuf+%d to rbuf+%d\n", *pupsize,
                        offset * count * extent);
        }

        offset += 1;
        nconsecutive_occurrences -= 1;

        if (nconsecutive_occurrences == 0) {    /*consecutive occurrences are over */
            offset += delta;
            nconsecutive_occurrences = pow_k_phase;
        }

        *pupsize += count * extent;     /*NOTE: This may not be extent, it might be type_size - CHECK THIS */
    }

    int wait_id = TSP_wait_for(s, counter, dtcopy_id);
    TSP_free_mem(dtcopy_id);
    return wait_id;
}

static inline int
COLL_sched_alltoall_brucks(const void *sendbuf, int sendcount, COLL_dt_t sendtype,
                           void *recvbuf, int recvcount, COLL_dt_t recvtype,
                           COLL_comm_t * comm, int tag, TSP_sched_t * s, int k)
{

    int *pack_invtcs = (int *) TSP_allocate_mem(sizeof(int) * k);
    int pack_ninvtcs;
    int *send_invtcs = (int *) TSP_allocate_mem(sizeof(int) * k);
    int send_ninvtcs;
    int *recv_invtcs = (int *) TSP_allocate_mem(sizeof(int) * k);
    int recv_ninvtcs;
    int *unpack_invtcs = (int *) TSP_allocate_mem(sizeof(int) * k);
    int unpack_ninvtcs;


    int *invtcs = (int *) TSP_allocate_mem(sizeof(int) * 6 * k);
    int n_invtcs;

    int rank = TSP_rank(&comm->tsp_comm);
    int size = TSP_size(&comm->tsp_comm);

    int nphases = 0;
    int max = size - 1;

    MPIC_DBG ("alltoall_brucks: num_ranks: %d, k: %d \n", size, k);
    /*calculate the number of bits required to represent a rank in base k */
    while (max) {
        nphases++;
        max /= k;
    }

    MPIC_DBG( "num phases: %d\n", nphases);

    if (TSP_isinplace(sendbuf)) {
        sendcount = recvcount;
        sendtype = recvtype;
    }

    /*get dt info of sendtype and recvtype */
    size_t s_type_size, s_extent, s_lb, r_type_size, r_extent, r_lb;
    int s_iscontig, r_iscontig;
    TSP_dtinfo(sendtype, &s_iscontig, &s_type_size, &s_extent, &s_lb);
    TSP_dtinfo(recvtype, &r_iscontig, &r_type_size, &r_extent, &r_lb);

    void *tmp_buf = (void *) TSP_allocate_buffer(
                                recvcount * size * r_extent, s);
    /*temporary buffer used for rotation
     * also used as sendbuf when inplace is true */
    const void *senddata;       /*pointer to send data */

    if (TSP_isinplace(sendbuf)) {
        /*copy from recvbuf to tmp_buf */
        invtcs[0] = TSP_dtcopy_nb(tmp_buf, size * recvcount, recvtype,
                                  recvbuf, size * recvcount, recvtype,
                                  s, 0, NULL);
        senddata = tmp_buf;

        n_invtcs = 1;
    }
    else {
        senddata = sendbuf;
        n_invtcs = 0;
    }

    /*Brucks algo Step 1: rotate the data locally */
    TSP_dtcopy_nb(recvbuf, (size - rank) * recvcount, recvtype,
                  (void *) ((char *) senddata + rank * sendcount * s_extent),
                  (size - rank) * sendcount, sendtype, s, n_invtcs, invtcs);
    TSP_dtcopy_nb((void *) ((char *) recvbuf + (size - rank) * recvcount * r_extent),
                  rank * recvcount, recvtype, senddata,
                  rank * sendcount, sendtype, s,
                  n_invtcs, invtcs);
    MPIC_DBG("Step 1 data rotation scheduled\n");

    TSP_wait(s);
    /*Step 2: Allocate buffer space for packing of data */
    int i, delta = 1, src, dst, j;
    void **tmp_sbuf = (void **) TSP_allocate_mem(sizeof(void *) * (k - 1));
    void **tmp_rbuf = (void **) TSP_allocate_mem(sizeof(void *) * (k - 1));

    for (i = 0; i < k - 1; i++) {
        tmp_sbuf[i] =
            TSP_allocate_buffer(
                    (int) r_extent * recvcount * ceil((float) size / k), s);
        tmp_rbuf[i] =
            TSP_allocate_buffer(
                    (int) r_extent * recvcount * ceil((float) size / k), s);
    }

    MPIC_DBG( "allocated temporary buffer space for packing\n");

    /*This is TSP_dt for packed buffer (for referring to one byte sized elements,
     * currently just using control_dt which is MPI_CHAR in mpich transport */
    TSP_dt_t *pack_dt = &TSP_global.control_dt;
    /*use invtcs in the following manner
     * 0..k-2 for pack ids
     * k-1..2k-3 for send ids
     * 2k-2..3k-4 for recv ids
     * 3k-3..4k-5 for unpack ids
     */
    int *packids = invtcs;
    int *sendids = invtcs + k - 1;
    int *recvids = invtcs + 2 * k - 2;
    int *unpackids = invtcs + 3 * k - 3;

    int packsize = 0;
    pack_ninvtcs = recv_ninvtcs = 0;

    for (i = 0; i < nphases; i++) {
        for (j = 1; j < k; j++) {       /*for every non-zero value of digitval */
            if (pow(k, i) * j >= size)  /*if the first location exceeds comm size, nothing is to be sent */
                break;

            src = (rank - delta * j + size) % size;
            dst = (rank + delta * j) % size;

            if (i != 0) {
                pack_invtcs[k - 1] = sendids[j - 1];
                pack_ninvtcs = k;
            }
            packids[j - 1] =
                COLL_brucks_pup(1, recvbuf, tmp_sbuf[j - 1],
                                recvtype, recvcount, i, k, j, size,
                                &packsize, s, pack_ninvtcs, pack_invtcs);
            *unpack_invtcs = packids[j - 1];
            unpack_ninvtcs = 1;
            MPIC_DBG("phase %d, digit %d packing scheduled\n", i, j);

            sendids[j - 1] =
                TSP_send(tmp_sbuf[j - 1], packsize, *pack_dt, dst, tag,
                        &comm->tsp_comm, s, 1, &packids[j - 1]);
            MPIC_DBG("phase %d, digit %d send scheduled\n", i, j);

            if (i != 0) {
                *recv_invtcs = unpackids[j - 1];
                recv_ninvtcs = 1;
            }
            recvids[j - 1] =
                TSP_recv( tmp_rbuf[j - 1], packsize, *pack_dt,
                          src, tag, &comm->tsp_comm,
                          s, recv_ninvtcs, recv_invtcs);
            MPIC_DBG("phase %d, digit %d recv scheduled\n", i, j);

            *(unpack_invtcs + 1) = recvids[j - 1];
            unpack_ninvtcs = 2;
            unpackids[j - 1] =
                COLL_brucks_pup(0, recvbuf, tmp_rbuf[j - 1], recvtype,
                                recvcount, i, k, j, size,
                                &packsize, s, unpack_ninvtcs, unpack_invtcs);
            MPIC_DBG( "phase %d, digit %d unpacking scheduled\n", i, j);
        }
        TSP_dtcopy(pack_invtcs, sizeof(int) * (k - 1), *pack_dt,
                    unpackids, sizeof(int) * (k - 1), *pack_dt);
        pack_ninvtcs = k - 1;

        MPIC_DBG( "phase %d scheduled\n", i);

        delta *= k;
    }

    MPIC_DBG( "Step 2 %d scheduled\n", i);

    /*Step 3: rotate the buffer */
    /*TODO: MPICH implementation does some lower_bound adjustment
     * here for derived datatypes, I am skipping that for now,
     * will come back to it later on - will require adding API
     * for getting true_lb */
    invtcs[0] = TSP_dtcopy_nb(tmp_buf, (size - rank - 1) * recvcount, recvtype,
                              (void *) ((char *) recvbuf
                                  + (rank + 1) * recvcount * r_extent),
                              (size - rank - 1) * recvcount, recvtype, s,
                              k - 1, unpackids);
    invtcs[1] =
        TSP_dtcopy_nb((void *) ((char *) tmp_buf 
                            + (size - rank - 1) * recvcount * r_extent),
                      (rank + 1) * recvcount, recvtype, recvbuf,
                      (rank + 1) * recvcount, recvtype,
                      s, k - 1, unpackids);

    /*invert the buffer now to get the result in desired order */
    for (i = 0; i < size; i++)
        TSP_dtcopy_nb((void *) ((char *) recvbuf 
                                + (size - i - 1) * recvcount * r_extent),
                      recvcount, recvtype, tmp_buf + i * recvcount * r_extent,
                      recvcount, recvtype, s, 2, invtcs);

    MPIC_DBG("Step 3: data rearrangement scheduled\n");

    /*TSP_free_mem(tmp_sbuf);
     * TSP_free_mem(tmp_rbuf);
     * TSP_free_mem(invtcs);
     * TSP_free_mem(pack_invtcs);
     * TSP_free_mem(unpack_invtcs);
     * TSP_free_mem(send_invtcs);
     * TSP_free_mem(recv_invtcs); */
    return 0;
}

static inline int COLL_sched_barrier_dissem(int tag, COLL_comm_t * comm, TSP_sched_t * s)
{
    int i, n;
    int nphases = 0;
    TSP_dt_t dt = ((TSP_global).control_dt);

    for (n = comm->nranks - 1; n > 0; n >>= 1)
        nphases++;
    MPIC_DBG("dissem barrier - nphases = %d\n", nphases);

    int *recvids = TSP_allocate_mem(sizeof(int) * nphases);
    for (i = 0; i < nphases; i++) {
        MPIC_DBG("dissem barrier - start scheduling phase %d\n", i);
        int shift = (1 << i);
        int to = (comm->rank + shift) % comm->nranks;
        int from = (comm->rank) - shift;

        if (from < 0)
            from += comm->nranks;

        MPIC_DBG("dissem barrier - scheduling recv phase %d\n", i);
        recvids[i] = TSP_recv(NULL, 0, dt, from, tag, &comm->tsp_comm, s, 0, NULL);
        MPIC_DBG("dissem barrier - scheduling send phase %d\n", i);
        TSP_send(NULL, 0, dt, to, tag, &comm->tsp_comm, s, i, recvids);
        MPIC_DBG("dissem barrier - scheduled phase %d\n", i);
    }

    TSP_sched_commit(s);
    TSP_free_mem(recvids);
    MPIC_DBG("dissem barrier - finished scheduling\n");
    return 0;
}

static inline int
COLL_sched_allreduce_dissem(const void *sendbuf,
                            void *recvbuf,
                            int count,
                            COLL_dt_t datatype,
                            COLL_op_t op, int tag,
                            COLL_comm_t * comm, TSP_sched_t * s)
{
    /* does not handle in place or communative */
    int upperPow, lowerPow, nphases = 0;
    int i, n, is_contig, notPow2, inLower, dissemPhases, dissemRanks;
    size_t extent, lb, type_size;

    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);

    for (n = comm->nranks - 1; n > 0; n >>= 1)
        nphases++;

    upperPow = (1 << nphases);
    lowerPow = (1 << (nphases - 1));
    notPow2 = (upperPow != comm->nranks);

    int dtcopy_id = TSP_dtcopy_nb(recvbuf, count, datatype,
                                  sendbuf, count, datatype,
                                  s, 0, NULL);

    inLower = comm->rank < lowerPow;
    dissemPhases = nphases - 1;
    dissemRanks = lowerPow;

    int rrid = -1, sid;
    /* recv_reduce id and send id for supporting DAG */
    if (notPow2 && inLower) {
        int from = comm->rank + lowerPow;

        if (from < comm->nranks) {
            rrid = TSP_recv_reduce(recvbuf, count, datatype,
                                   op, from, tag, &comm->tsp_comm,
                                   TSP_FLAG_REDUCE_L,
                                   s, 1, &dtcopy_id);
        }
    }
    else if (notPow2) {
        int to = comm->rank % lowerPow;
        TSP_send_accumulate(sendbuf, count, datatype,
                            op, to, tag, &comm->tsp_comm, s, 0, NULL);
    }
    else {
        inLower = 1;
        dissemPhases = nphases;
        dissemRanks = comm->nranks;
    }
    int id[2];
    id[0] = (rrid == -1) ? dtcopy_id : rrid;
    if (inLower) {
        void *tmpbuf = TSP_allocate_buffer(extent * count, s);
        for (i = 0; i < dissemPhases; i++) {
            int shift = (1 << i);
            int to = (comm->rank + shift) % dissemRanks;
            int from = (comm->rank) - shift;

            if (from < 0)
                from = dissemRanks + from;

            dtcopy_id = TSP_dtcopy_nb(tmpbuf, count, datatype,
                                      recvbuf, count, datatype,
                                      s, (i == 0) ? 1 : 2, id);
            id[0] = TSP_send_accumulate(tmpbuf, count, datatype,
                                        op, to, tag, &comm->tsp_comm,
                                        s, 1, &dtcopy_id);
            id[1] = TSP_recv_reduce(recvbuf, count, datatype,
                                    op, from, tag, &comm->tsp_comm,
                                    TSP_FLAG_REDUCE_L, s, 1, &dtcopy_id);
        }
    }

    if (notPow2 && inLower) {
        int to = comm->rank + lowerPow;

        if (to < comm->nranks) {
            TSP_send(recvbuf, count, datatype, to, tag,
                        &comm->tsp_comm, s, 1, id + 1);
        }
    }
    else if (notPow2) {
        int from = comm->rank % lowerPow;
        TSP_recv(recvbuf, count, datatype, from, tag,
                &comm->tsp_comm, s, 0, NULL);
    }
    return 0;
}