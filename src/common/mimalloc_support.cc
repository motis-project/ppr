#if PPR_USE_MIMALLOC

#include "mimalloc.h"
#include "mimalloc-new-delete.h"

namespace ppr {

void init_mimalloc() { mi_version(); }

}  // namespace ppr

#else

namespace ppr {

void init_mimalloc() {}

}  // namespace ppr

#endif
