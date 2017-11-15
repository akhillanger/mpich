ext_coll_tests=""


nl='
'
testing_env="env=MPIR_CVAR_BCAST_DEVICE_COLLECTIVE=0"
algo_names="generic_tree_kary_nb generic_tree_knomial_nb"
kvalues="3"
for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        env="env=MPIR_CVAR_BCAST_ALGORITHM_INTRA=${algo_name} "
        env+="env=MPIR_CVAR_BCAST_TREE_KVAL=${kval} ${testing_env}"
        ext_coll_tests+="bcasttest 10 ${env}${nl}"
        ext_coll_tests+="bcast_full 4 timeLimit=600 ${env}${nl}"
        ext_coll_tests+="bcast_min_datatypes 10 timeLimit=1200 ${env}${nl}"
        ext_coll_tests+="bcast_comm_world 10 timeLimit=1200 ${env}${nl}"
        ext_coll_tests+="bcastzerotype 10 ${env}${nl}"
    done
done
# Add more tests
export ext_coll_tests
