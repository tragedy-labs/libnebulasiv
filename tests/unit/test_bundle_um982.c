// test_bundle_um982.c
//
// Compile-time regression guard for the UM982 bundle header (see
// test_bundle_um960.c for the rationale). UM982 is the fullest model: the six
// common modules plus AGNSS assist and the complete dual-antenna heading set.
#include "um982.h"

#if !defined(NEB_CORE_H) || !defined(NEB_MODE_H) || !defined(NEB_CONFIG_H) ||  \
    !defined(NEB_RTK_H) || !defined(NEB_MASK_H) || !defined(NEB_ADMIN_H) ||    \
    !defined(NEB_LOGGING_H) || !defined(NEB_ASSIST_H) || !defined(NEB_HEADING_H)
#error "um982.h no longer exposes the full UM982 module set"
#endif

int neb_bundle_um982_ok = 1;
