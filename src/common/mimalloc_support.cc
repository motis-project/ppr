#if PPR_USE_MIMALLOC

#include "mimalloc-new-delete.h"
#include "mimalloc.h"

namespace ppr {

void init_mimalloc() { mi_version(); }

}  // namespace ppr

#else

namespace ppr {

void init_mimalloc() {}

}  // namespace ppr

#endif
