// test_bundle_um960.c
//
// Compile-time regression guard for the UM960 bundle header. Including um960.h
// alone must expose the model's full capability-module set; if a future edit
// drops one of these includes, this translation unit fails to compile (the
// guard macro is undefined) and `make test` breaks before any test runs. It
// also asserts the two model-specific modules stay OUT, so the header does not
// silently over-include. This is a pure preprocessor check -- no runtime test.
#include "um960.h"

#if !defined(NEB_CORE_H) || !defined(NEB_MODE_H) || !defined(NEB_CONFIG_H) ||  \
    !defined(NEB_RTK_H) || !defined(NEB_MASK_H) || !defined(NEB_ADMIN_H) ||    \
    !defined(NEB_LOGGING_H)
#error "um960.h no longer exposes the full UM960 module set"
#endif

#if defined(NEB_ASSIST_H) || defined(NEB_HEADING_H)
#error "um960.h should not include AGNSS assist or heading (unsupported on UM960)"
#endif

// Non-empty TU (ISO C / -Wpedantic): a marker with external linkage.
int neb_bundle_um960_ok = 1;
