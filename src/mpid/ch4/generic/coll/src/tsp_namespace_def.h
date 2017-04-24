#define TSP_NSCAT0(a,b) a##b
#define TSP_NSCAT1(a,b) TSP_NSCAT0(a,b)
#define TSP_NAMESPACE(fn)   TSP_NSCAT1(\
                            TSP_NSCAT1(\
                            TSP_NSCAT1(\
                                GLOBAL_NAME,         \
                                TRANSPORT_),         \
                                TRANSPORT_NAME),     \
                                fn)
/* Transport Types */
#define TSP_dt_t             TSP_NAMESPACE(dt_t)
#define TSP_op_t             TSP_NAMESPACE(op_t)
#define TSP_comm_t           TSP_NAMESPACE(comm_t)
#define TSP_req_t            TSP_NAMESPACE(req_t)
#define TSP_sched_t          TSP_NAMESPACE(sched_t)
#define TSP_aint_t           TSP_NAMESPACE(aint_t)
#define TSP_global_t         TSP_NAMESPACE(global_t)
#define TSP_global           MPIDI_COLL_global_instance.TSP_NSCAT1(tsp_,TRANSPORT_NAME_LC)
/* NB transport argument types */
#define TSP_sendrecv_arg_t       TSP_NAMESPACE(sendrecv_arg_t)
#define TSP_recv_reduce_arg_t    TSP_NAMESPACE(recv_reduce_arg_t)
#define TSP_addref_dt_arg_t      TSP_NAMESPACE(addref_dt_arg_t)
#define TSP_addref_op_arg_t      TSP_NAMESPACE(addref_op_arg_t)
#define TSP_dtcopy_arg_t         TSP_NAMESPACE(dtcopy_arg_t)
#define TSP_free_mem_arg_t       TSP_NAMESPACE(free_mem_arg_t)
#define TSP_reduce_local_arg_t   TSP_NAMESPACE(reduce_local_arg_t)
#define TSP_KIND_SEND            TSP_NAMESPACE(TSP_KIND_SEND)
#define TSP_KIND_RECV            TSP_NAMESPACE(TSP_KIND_RECV)
#define TSP_KIND_ADDREF_DT       TSP_NAMESPACE(TSP_KIND_ADDREF_DT)
#define TSP_KIND_ADDREF_OP       TSP_NAMESPACE(TSP_KIND_ADDREF_OP)
#define TSP_KIND_DTCOPY          TSP_NAMESPACE(TSP_KIND_DTCOPY)
#define TSP_KIND_FREE_MEM        TSP_NAMESPACE(TSP_KIND_FREE_MEM)
#define TSP_KIND_RECV_REDUCE     TSP_NAMESPACE(TSP_KIND_RECV_REDUCE)
#define TSP_KIND_REDUCE_LOCAL    TSP_NAMESPACE(TSP_KIND_REDUCE_LOCAL)
#define TSP_STATE_INIT           TSP_NAMESPACE(TSP_STATE_INIT)
#define TSP_STATE_ISSUED         TSP_NAMESPACE(TSP_STATE_ISSUED)
#define TSP_STATE_COMPLETE       TSP_NAMESPACE(TSP_STATE_COMPLETE)
#define TSP_FLAG_REDUCE_L        (1ULL)
#define TSP_FLAG_REDUCE_R        (2ULL)

/* Transport APIS */
#define TSP_init             TSP_NAMESPACE(init)
#define TSP_comm_cleanup     TSP_NAMESPACE(comm_cleanup)
#define TSP_comm_init        TSP_NAMESPACE(comm_init)
#define TSP_dt_init          TSP_NAMESPACE(dt_init)
#define TSP_op_init          TSP_NAMESPACE(op_init)
#define TSP_sched_init       TSP_NAMESPACE(sched_init)
#define TSP_sched_reset       TSP_NAMESPACE(sched_reset)
#define TSP_sched_commit     TSP_NAMESPACE(sched_commit)
#define TSP_sched_start      TSP_NAMESPACE(sched_start)
#define TSP_sched_finalize   TSP_NAMESPACE(sched_finalize)
#define TSP_init_control_dt  TSP_NAMESPACE(init_control_dt)
#define TSP_fence            TSP_NAMESPACE(fence)
#define TSP_wait             TSP_NAMESPACE(wait)
#define TSP_wait4            TSP_NAMESPACE(wait4)
#define TSP_opinfo           TSP_NAMESPACE(opinfo)
#define TSP_isinplace        TSP_NAMESPACE(isinplace)
#define TSP_dtinfo           TSP_NAMESPACE(dtinfo)
#define TSP_addref_dt        TSP_NAMESPACE(addref_dt)
#define TSP_addref_dt_nb     TSP_NAMESPACE(addref_dt_nb)
#define TSP_addref_op        TSP_NAMESPACE(addref_op)
#define TSP_addref_op_nb     TSP_NAMESPACE(addref_op_nb)
#define TSP_send             TSP_NAMESPACE(send)
#define TSP_recv             TSP_NAMESPACE(recv)
#define TSP_recv_reduce      TSP_NAMESPACE(recv_reduce)
#define TSP_send_accumulate  TSP_NAMESPACE(send_accumulate)
#define TSP_test             TSP_NAMESPACE(test)
#define TSP_rank             TSP_NAMESPACE(rank)
#define TSP_size             TSP_NAMESPACE(size)
#define TSP_reduce_local     TSP_NAMESPACE(reduce_local)
#define TSP_dtcopy           TSP_NAMESPACE(dtcopy)
#define TSP_dtcopy_nb        TSP_NAMESPACE(dtcopy_nb)
#define TSP_allocate_mem     TSP_NAMESPACE(allocate_mem)
#define TSP_allocate_buffer  TSP_NAMESPACE(allocate_buffer)
#define TSP_free_mem         TSP_NAMESPACE(free_mem)
#define TSP_free_buffers     TSP_NAMESPACE(free_buffers)
#define TSP_free_mem_nb      TSP_NAMESPACE(free_mem_nb)
#define TSP_queryfcn         TSP_NAMESPACE(queryfcn)
#define TSP_record_request_completion   TSP_NAMESPACE(record_request_completion)
#define TSP_record_request_issue        TSP_NAMESPACE(record_request_issue)
#define TSP_issue_request               TSP_NAMESPACE(issue_request)
#define TSP_decrement_num_unfinished_dependencies TSP_NAMESPACE(decrement_num_unfinished_dependecies)

#define TSP_add_vtx_dependencies            TSP_NAMESPACE(add_vtx_dependencies)
