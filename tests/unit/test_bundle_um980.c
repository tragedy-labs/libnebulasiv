// test_bundle_um980.c
//
// Compile-time regression guard for the UM980 bundle header (see
// test_bundle_um980's sibling test_bundle_um960.c for the rationale). UM980
// adds AGNSS assist and the single-antenna HEADING OFFSET module on top of the
// six common modules.
#include "um980.h"

#if !defined(NEB_CORE_H) || !defined(NEB_MODE_H) || !defined(NEB_CONFIG_H) ||  \
    !defined(NEB_RTK_H) || !defined(NEB_MASK_H) || !defined(NEB_ADMIN_H) ||    \
    !defined(NEB_LOGGING_H) || !defined(NEB_ASSIST_H) || !defined(NEB_HEADING_H)
#error "um980.h no longer exposes the full UM980 module set"
#endif

int neb_bundle_um980_ok = 1;
