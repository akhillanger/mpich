#define COLL_USE_KNOMIAL       0
#define COLL_TREE_RADIX_DEFAULT 2
#define COLL_MAX_TREE_BREADTH  16
#define COLL_NAME KARY_
#define COLL_NAME_LC kary
#include "../../src/coll_namespace_def.h"
#include "../common/common_fns.h"
#include "../common/schedule_tree.h"
#include "./coll.h"
#include "../../src/coll_namespace_undef.h"
#undef COLL_USE_KNOMIAL
#undef COLL_TREE_RADIX_DEFAULT
#undef COLL_MAX_TREE_BREADTH
