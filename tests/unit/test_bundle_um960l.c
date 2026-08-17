// test_bundle_um960l.c
//
// Compile-time regression guard for the UM960L bundle header (see
// test_bundle_um960.c for the rationale). UM960L exposes the same six modules
// as UM960 -- the narrower runtime capability set is enforced by the bitfield,
// not by omitting module headers -- and, like UM960, excludes assist/heading.
#include "um960l.h"

#if !defined(NEB_CORE_H) || !defined(NEB_MODE_H) || !defined(NEB_CONFIG_H) ||  \
    !defined(NEB_RTK_H) || !defined(NEB_MASK_H) || !defined(NEB_ADMIN_H) ||    \
    !defined(NEB_LOGGING_H)
#error "um960l.h no longer exposes the full UM960L module set"
#endif

#if defined(NEB_ASSIST_H) || defined(NEB_HEADING_H)
#error                                                                         \
    "um960l.h should not include AGNSS assist or heading (unsupported on UM960L)"
#endif

int neb_bundle_um960l_ok = 1;
