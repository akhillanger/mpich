#define COLL_USE_KNOMIAL       0
#define COLL_TREE_RADIX_DEFAULT 2
#define COLL_MAX_TREE_BREADTH  16
#include "../coll_namespace_pre.h"
#include "../../algo/include/common_impl.h"
#include "../../sched/schedule_tree.h"
#include "../../sched/schedule_ring.h"
#include "../../sched/schedule_scattered.h"
#include "../../sched/schedule_pairwise.h"
#include "../../algo/tree/coll_tree.h"
#include "../coll_namespace_post.h"
#undef COLL_USE_KNOMIAL
#undef COLL_TREE_RADIX_DEFAULT
#undef COLL_MAX_TREE_BREADTH