#define TRANSPORT_NAME BMPICH_
#define TRANSPORT_NAME_LC bmpich

#define TSP_sched_t     MPIC_BMPICH_sched_t
#define TSP_aint_t      MPIC_BMPICH_aint_t
#define TSP_comm_t      MPIC_BMPICH_comm_t
#define TSP_global_t    MPIC_BMPICH_global_t
#define TSP_dt_t        MPIC_BMPICH_dt_t
#define TSP_op_t        MPIC_BMPICH_op_t

#define TSP_global      ((MPIC_global_instance.tsp_bmpich))

#define TSP_init             MPIC_BMPICH_init
#define TSP_comm_cleanup     MPIC_BMPICH_comm_cleanup
#define TSP_comm_init        MPIC_BMPICH_comm_init
#define TSP_dt_init          MPIC_BMPICH_dt_init
#define TSP_op_init          MPIC_BMPICH_op_init
#define TSP_fence            MPIC_BMPICH_fence
#define TSP_wait             MPIC_BMPICH_wait
#define TSP_opinfo           MPIC_BMPICH_opinfo
#define TSP_isinplace        MPIC_BMPICH_isinplace
#define TSP_dtinfo           MPIC_BMPICH_dtinfo
#define TSP_addref_dt        MPIC_BMPICH_addref_dt
#define TSP_addref_dt_nb     MPIC_BMPICH_addref_dt_nb
#define TSP_addref_op        MPIC_BMPICH_addref_op
#define TSP_addref_op_nb     MPIC_BMPICH_addref_op_nb
#define TSP_send             MPIC_BMPICH_send
#define TSP_recv             MPIC_BMPICH_recv
#define TSP_recv_reduce      MPIC_BMPICH_recv_reduce
#define TSP_send_accumulate  MPIC_BMPICH_send_accumulate
#define TSP_test             MPIC_BMPICH_test
#define TSP_rank             MPIC_BMPICH_rank
#define TSP_size             MPIC_BMPICH_size
#define TSP_reduce_local     MPIC_BMPICH_reduce_local
#define TSP_dtcopy           MPIC_BMPICH_dtcopy
#define TSP_dtcopy_nb        MPIC_BMPICH_dtcopy_nb
#define TSP_allocate_mem     MPIC_BMPICH_allocate_mem
#define TSP_allocate_buffer  MPIC_BMPICH_allocate_buffer
#define TSP_free_mem         MPIC_BMPICH_free_mem
#define TSP_free_buffers     MPIC_BMPICH_free_buffers
#define TSP_free_mem_nb      MPIC_BMPICH_free_mem_nb

#define TSP_FLAG_REDUCE_L    MPIC_FLAG_REDUCE_L
#define TSP_FLAG_REDUCE_R    MPIC_FLAG_REDUCE_R

/* Schedule API*/
/* Create/load schedule, reset it in case of load */
#define TSP_get_schedule        MPIC_BMPICH_get_schedule
/* Push schedule into cache, in case of implemented cache */
#define TSP_save_schedule       MPIC_BMPICH_save_schedule
/*Mark schedule as ready for execution. */
#define TSP_sched_start         MPIC_BMPICH_sched_start
/* Release schedule (destroy in case of disabled caching) */
#define TSP_sched_finalize      MPIC_BMPICH_sched_finalize
#define TSP_sched_commit        MPIC_BMPICH_sched_commit