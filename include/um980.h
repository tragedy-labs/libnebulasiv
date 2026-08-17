// um980.h
//
// Bundle header for the Unicore UM980. Includes exactly the capability
// modules this model supports; contains no new logic. Open a handle with
// model NEB_MODEL_UM980 (see UM980_MODEL) so the capability bitfield is set
// correctly and unsupported commands are rejected at runtime.
//
// UM980 supports MODE, CONFIG (incl. PPP, AGNSS, ANTIJAM), RTK, MASK, ADMIN,
// LOGGING and AGNSS assist input. It has no dual-antenna heading, but it does
// support the single-antenna HEADING OFFSET command (Manual §4.9), so
// neb_heading.h is included -- the UM982-only HEADING commands within it are
// gated off at runtime.
#ifndef NEB_UM980_H
#define NEB_UM980_H

#include "neb_core.h"

#include "neb_admin.h"
#include "neb_assist.h"
#include "neb_config.h"
#include "neb_heading.h"
#include "neb_logging.h"
#include "neb_mask.h"
#include "neb_mode.h"
#include "neb_rtk.h"

#define UM980_MODEL NEB_MODEL_UM980

#endif
