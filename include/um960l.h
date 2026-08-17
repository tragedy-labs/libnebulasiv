// um960l.h
//
// Bundle header for the Unicore UM960L. Includes exactly the capability
// modules this model supports; contains no new logic. Open a handle with
// model NEB_MODEL_UM960L (see UM960L_MODEL) so the capability bitfield is set
// correctly and unsupported commands are rejected at runtime.
//
// UM960L supports MODE, CONFIG, RTK, MASK, ADMIN and LOGGING. Per the manual
// it does not carry the PPP / ANTIJAM / SBAS / AGNSS / heading capabilities of
// its siblings; those commands live in the modules below but are rejected at
// runtime via the capability bitfield. AGNSS assist and dual-antenna heading
// are unsupported entirely, so neb_assist.h / neb_heading.h are not included.
#ifndef NEB_UM960L_H
#define NEB_UM960L_H

#include "neb_core.h"

#include "neb_admin.h"
#include "neb_config.h"
#include "neb_logging.h"
#include "neb_mask.h"
#include "neb_mode.h"
#include "neb_rtk.h"

#define UM960L_MODEL NEB_MODEL_UM960L

#endif
