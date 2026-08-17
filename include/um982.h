// um982.h
//
// Bundle header for the Unicore UM982, the dual-antenna model. Includes
// exactly the capability modules this model supports; contains no new logic.
// Open a handle with model NEB_MODEL_UM982 (see UM982_MODEL) so the capability
// bitfield is set correctly and unsupported commands are rejected at runtime.
//
// UM982 supports MODE, CONFIG (incl. PPP, AGNSS), RTK, MASK, ADMIN, LOGGING,
// AGNSS assist input and dual-antenna HEADING (Manual §4.8) plus the
// single-antenna HEADING OFFSET (§4.9). It is the only model with the full
// heading command set.
#ifndef NEB_UM982_H
#define NEB_UM982_H

#include "neb_core.h"

#include "neb_admin.h"
#include "neb_assist.h"
#include "neb_config.h"
#include "neb_heading.h"
#include "neb_logging.h"
#include "neb_mask.h"
#include "neb_mode.h"
#include "neb_rtk.h"

#define UM982_MODEL NEB_MODEL_UM982

#endif
