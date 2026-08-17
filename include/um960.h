// um960.h
//
// Bundle header for the Unicore UM960. Includes exactly the capability
// modules this model supports; contains no new logic. Open a handle with
// model NEB_MODEL_UM960 (see UM960_MODEL) so the capability bitfield is set
// correctly and unsupported commands are rejected at runtime.
//
// UM960 supports MODE, CONFIG (incl. ANTIJAM), RTK, MASK, ADMIN and LOGGING.
// It does not support AGNSS assist input or dual-antenna heading, so
// neb_assist.h / neb_heading.h are intentionally not included.
#ifndef NEB_UM960_H
#define NEB_UM960_H

#include "neb_core.h"

#include "neb_admin.h"
#include "neb_config.h"
#include "neb_logging.h"
#include "neb_mask.h"
#include "neb_mode.h"
#include "neb_rtk.h"

#define UM960_MODEL NEB_MODEL_UM960

#endif
