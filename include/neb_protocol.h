// neb_protocol.h
//
// Single source of truth for wire-format strings and closed-vocabulary
// enums used to talk to Unicore NebulasIV (N4) receivers.
//
// If Unicore changes command casing/spelling in a firmware revision, this
// file (and only this file) should need editing. No command-builder in the
// capability modules may embed a literal command token; they all pull from
// the macros here.
//
// Casing follows the manual's own examples verbatim (e.g. "Enable"/"Disable"
// for PPP). The receiver's parser is case-INSENSITIVE for headers, config
// items, and value tokens (verified on UM980 R4.10 Build13504: "MODE"/"mode",
// "AGNSS"/"agnss", "Disable"/"DISABLE"/"DiSaBlE" all accepted), so mirroring
// the manual's casing is for traceability, not acceptance. Token *spelling* and
// *word spacing* ARE load-bearing -- the device rejects those -- so the
// exact-string tests still guard the parts that matter.
#ifndef NEB_PROTOCOL_H
#define NEB_PROTOCOL_H

#include <stddef.h>

// ---------------------------------------------------------------------------
// Framing
// ---------------------------------------------------------------------------
// Abbreviated ASCII input has NO CRC (Manual §1). Commands are terminated
// with CR+LF.
#define NEB_LINE_TERMINATOR "\r\n"

// ---------------------------------------------------------------------------
// Command headers (Manual §2, Table 2-1)
// ---------------------------------------------------------------------------
#define NEB_CMD_MODE "MODE"        // Manual §3
#define NEB_CMD_CONFIG "CONFIG"    // Manual §4
#define NEB_CMD_MASK "MASK"        // Manual §5.1/§5.2
#define NEB_CMD_UNMASK "UNMASK"    // Manual §5.3
#define NEB_TOK_PRN "PRN"          // Manual §5.2/§5.3 (per-satellite mask)
#define NEB_TOK_RTCMCN0 "RTCMCN0"  // Manual §5.2, Table 5-5
#define NEB_TOK_CN0 "CN0"          // Manual §5.2, Table 5-5
#define NEB_CMD_AIDPOS "$AIDPOS"   // Manual §6.1 (NMEA-style, $-prefixed)
#define NEB_CMD_AIDTIME "$AIDTIME" // Manual §6.2
#define NEB_CMD_UNLOG "UNLOG"      // Manual §8.1
#define NEB_CMD_FRESET "FRESET"    // Manual §8.2
#define NEB_CMD_RESET "RESET"      // Manual §8.3
#define NEB_CMD_SAVECONFIG "SAVECONFIG" // Manual §8.4
#define NEB_TOK_ALL "ALL"               // Manual §8.3
#define NEB_TOK_EPHEM "EPHEM"           // Manual §8.3
#define NEB_TOK_ALMANAC "ALMANAC"       // Manual §8.3
#define NEB_TOK_IONUTC "IONUTC"         // Manual §8.3
#define NEB_TOK_POSITION "POSITION"     // Manual §8.3
#define NEB_TOK_XOPARAM "XOPARAM"       // Manual §8.3 (a.k.a. CLOCKDRIFT)
#define NEB_TOK_ONCHANGED "ONCHANGED"   // Manual §7 (output-on-change trigger)

// ---------------------------------------------------------------------------
// MODE tokens (Manual §3)
// ---------------------------------------------------------------------------
#define NEB_TOK_BASE "BASE"         // Manual §3.2
#define NEB_TOK_ROVER "ROVER"       // Manual §3.6
#define NEB_TOK_HEADING2 "HEADING2" // Manual §3.7
#define NEB_TOK_TIME "TIME"         // Manual §3.3 (self-optimizing base)

// ---------------------------------------------------------------------------
// CONFIG configuration items (Manual §4)
// ---------------------------------------------------------------------------
#define NEB_TOK_PPP "PPP"           // Manual §4.19
#define NEB_TOK_CONVERGE "CONVERGE" // Manual §4.19, Table 4-25
#define NEB_TOK_ENABLE "Enable"     // Manual §4.19 example casing
#define NEB_TOK_DISABLE "Disable"   // Manual §4.19 example casing
#define NEB_TOK_NMEA0183                                                       \
  "NMEA0183"                      // Manual §4.14 (setter token; the query
                                  // echoes "NMEAVERSION")
#define NEB_TOK_ANTIJAM "ANTIJAM" // Manual §4.20
#define NEB_TOK_AGNSS "AGNSS"     // Manual §4.18
#define NEB_TOK_UNDULATION "UNDULATION" // Manual §4.4
#define NEB_TOK_AUTO "Auto"             // Manual §4.4 example casing
#define NEB_TOK_SMOOTH "SMOOTH"         // Manual §4.12
#define NEB_TOK_RTKHEIGHT "RTKHEIGHT"   // Manual §4.12
#define NEB_TOK_HEADING "HEADING"       // Manual §4.8 / §4.12
#define NEB_TOK_OFFSET "OFFSET"         // Manual §4.9
#define NEB_TOK_LENGTH "LENGTH"         // Manual §4.8
#define NEB_TOK_PSRVEL "PSRVEL"         // Manual §4.12
#define NEB_TOK_ENABLE_LC "enable"      // Manual §4.12 example casing (lower)
#define NEB_TOK_DISABLE_LC "disable"    // Manual §4.12 example casing (lower)
#define NEB_TOK_RTCMB1CB2A "RTCMB1CB2a" // Manual §4.15 (exact casing)
#define NEB_TOK_IONMODE "IONMODE"       // Manual §4.25
#define NEB_TOK_MMP "MMP"               // Manual §4.13
#define NEB_TOK_EVENT "EVENT"           // Manual §4.11
#define NEB_TOK_ENABLE_UC "ENABLE" // Manual §4.13/§4.11 example casing (upper)
#define NEB_TOK_DISABLE_UC "DISABLE" // Manual §4.13/§4.11 example casing
#define NEB_TOK_RTCMPHASERATE "RTCMPHASERATE" // Manual §4.16
#define NEB_TOK_PSRVELDRPOS "PSRVELDRPOS"     // Manual §4.17
#define NEB_TOK_POSITIVE "POSITIVE"           // Manual §4.11/§4.16/§4.3
#define NEB_TOK_NEGATIVE "NEGATIVE"           // Manual §4.11/§4.16/§4.3
#define NEB_TOK_PPS "PPS"                     // Manual §4.3
#define NEB_TOK_DGPS "DGPS"                   // Manual §4.5
#define NEB_TOK_RTK "RTK"                     // Manual §4.6
#define NEB_TOK_TIMEOUT "TIMEOUT"             // Manual §4.5/§4.6
#define NEB_TOK_RELIABILITY "RELIABILITY"     // Manual §4.6
#define NEB_TOK_USER_DEFAULTS                                                  \
  "USER_DEFAULTS"                       // Manual §4.6 (verified on device)
#define NEB_TOK_RESET "RESET"           // Manual §4.6
#define NEB_TOK_STANDALONE "STANDALONE" // Manual §4.7
#define NEB_TOK_SBAS "SBAS"             // Manual §4.10
#define NEB_TOK_BASEANTENNAMODEL "BASEANTENNAMODEL" // Manual §4.26
#define NEB_TOK_ANTENNADELTAHEN "ANTENNADELTAHEN"   // Manual §4.22
#define NEB_TOK_SIGNALGROUP "SIGNALGROUP"           // Manual §4.21
#define NEB_TOK_ALGRESET "ALGRESET"                 // Manual §4.24

// ---------------------------------------------------------------------------
// Response framing (observed from hardware; not documented in the PDF)
// ---------------------------------------------------------------------------
// Every command is acknowledged with a line of the form:
//   $command,<echoed command>,response: OK*hh          (success)
//   $command,<echoed command>,response: PARSING FAILED ...,*hh   (rejected)
// A completely unknown command header produces no reply at all.
#define NEB_RESP_PREFIX "$command,"
#define NEB_RESP_OK "response: OK"
#define NEB_RESP_FAIL "PARSING FAILED"

// ---------------------------------------------------------------------------
// Closed-vocabulary value enums
// ---------------------------------------------------------------------------

// PPP correction source (Manual §4.19, Table 4-24, Parameter 2 of "Enable").
typedef enum {
  NEB_PPP_B2B,   // B2b-PPP (default)
  NEB_PPP_SSR_RX // RXN PPP SSR service
} neb_ppp_mode_t;

// Wire string for a PPP mode. Returns NULL for an out-of-range value so
// callers can treat it as an invalid parameter.
static inline const char *neb_ppp_mode_str(neb_ppp_mode_t mode) {
  switch (mode) {
  case NEB_PPP_B2B:
    return "B2b-PPP"; // Manual §4.19, Table 4-24
  case NEB_PPP_SSR_RX:
    return "SSR-RX"; // Manual §4.19, Table 4-24
  default:
    return NULL;
  }
}

// Rover dynamic profile for MODE ROVER (Manual §3.6, Table 3-8). Each value is
// a valid parameter-1/parameter-2 combination from the manual; invalid pairings
// (e.g. UAV with the lawn-mower sub-mode) simply do not exist as enum values.
typedef enum {
  NEB_ROVER_UAV,           // UAV, default sub-mode
  NEB_ROVER_UAV_FORMATION, // UAV, formation sub-mode
  NEB_ROVER_SURVEY,        // precision surveying, default sub-mode
  NEB_ROVER_SURVEY_MOW,    // precision surveying, lawn-mower sub-mode
  NEB_ROVER_AUTOMOTIVE     // automotive, default sub-mode
} neb_rover_profile_t;

// Wire string for a rover profile, e.g. "SURVEY MOW". Returns NULL for an
// out-of-range value so callers can treat it as an invalid parameter.
static inline const char *neb_rover_profile_str(neb_rover_profile_t profile) {
  switch (profile) {
  case NEB_ROVER_UAV:
    return "UAV"; // Manual §3.6, Table 3-8
  case NEB_ROVER_UAV_FORMATION:
    return "UAV FORMATION"; // Manual §3.6, Table 3-8
  case NEB_ROVER_SURVEY:
    return "SURVEY"; // Manual §3.6, Table 3-8
  case NEB_ROVER_SURVEY_MOW:
    return "SURVEY MOW"; // Manual §3.6, Table 3-8
  case NEB_ROVER_AUTOMOTIVE:
    return "AUTOMOTIVE"; // Manual §3.6, Table 3-8
  default:
    return NULL;
  }
}

// Baseline model for MODE HEADING2 (Manual §3.7, Table 3-10). Describes how the
// baseline between the two antennas behaves. An empty parameter on the wire
// defaults to FIXLENGTH.
typedef enum {
  NEB_HEADING2_FIXLENGTH,      // fixed baseline length (default)
  NEB_HEADING2_VARIABLELENGTH, // baseline length changes dynamically
  NEB_HEADING2_STATIC,         // both antennas static
  NEB_HEADING2_LOWDYNAMIC,     // low-dynamic carriers (e.g. pile drivers)
  NEB_HEADING2_TRACTOR         // agricultural-machinery operating mode
} neb_heading2_mode_t;

// Wire string for a heading2 baseline model. Returns NULL for an out-of-range
// value so callers can treat it as an invalid parameter.
static inline const char *neb_heading2_mode_str(neb_heading2_mode_t mode) {
  switch (mode) {
  case NEB_HEADING2_FIXLENGTH:
    return "FIXLENGTH"; // Manual §3.7, Table 3-10
  case NEB_HEADING2_VARIABLELENGTH:
    return "VARIABLELENGTH"; // Manual §3.7, Table 3-10
  case NEB_HEADING2_STATIC:
    return "STATIC"; // Manual §3.7, Table 3-10
  case NEB_HEADING2_LOWDYNAMIC:
    return "LOWDYNAMIC"; // Manual §3.7, Table 3-10
  case NEB_HEADING2_TRACTOR:
    return "TRACTOR"; // Manual §3.7, Table 3-10
  default:
    return NULL;
  }
}

// Serial port for CONFIG COMx (Manual §4.2). The receiver has three
// independent ports.
typedef enum { NEB_COM1, NEB_COM2, NEB_COM3 } neb_com_port_t;

// Wire string for a serial port. Returns NULL for an out-of-range value.
static inline const char *neb_com_port_str(neb_com_port_t port) {
  switch (port) {
  case NEB_COM1:
    return "COM1"; // Manual §4.2, Table 4-3
  case NEB_COM2:
    return "COM2"; // Manual §4.2, Table 4-3
  case NEB_COM3:
    return "COM3"; // Manual §4.2, Table 4-3
  default:
    return NULL;
  }
}

// NMEA 0183 output version for CONFIG NMEA0183 (Manual §4.14, Table 4-19).
typedef enum {
  NEB_NMEA_V410, // V410 (default; extended to support BDS)
  NEB_NMEA_V411  // V411
} neb_nmea_version_t;

// Wire string for an NMEA version. Returns NULL for an out-of-range value.
static inline const char *neb_nmea_version_str(neb_nmea_version_t version) {
  switch (version) {
  case NEB_NMEA_V410:
    return "V410"; // Manual §4.14, Table 4-19
  case NEB_NMEA_V411:
    return "V411"; // Manual §4.14, Table 4-19
  default:
    return NULL;
  }
}

// Anti-jamming mode for CONFIG ANTIJAM (Manual §4.20, Table 4-26).
typedef enum {
  NEB_ANTIJAM_DISABLE, // disable anti-jamming
  NEB_ANTIJAM_AUTO,    // autonomous (default)
  NEB_ANTIJAM_FORCE    // forced (higher power consumption)
} neb_antijam_mode_t;

// Wire string for an anti-jamming mode. Returns NULL for an out-of-range value.
static inline const char *neb_antijam_mode_str(neb_antijam_mode_t mode) {
  switch (mode) {
  case NEB_ANTIJAM_DISABLE:
    return "DISABLE"; // Manual §4.20, Table 4-26
  case NEB_ANTIJAM_AUTO:
    return "AUTO"; // Manual §4.20, Table 4-26
  case NEB_ANTIJAM_FORCE:
    return "FORCE"; // Manual §4.20, Table 4-26
  default:
    return NULL;
  }
}

// Ionospheric model for CONFIG IONMODE (Manual §4.25, Table 4-33). BD2K8 and
// BD3GIM are documented tokens but marked "not supported currently" -- the
// builder still formats them correctly; the device decides whether to accept.
typedef enum {
  NEB_IONMODE_GPSK8,  // GPS ionospheric model (default)
  NEB_IONMODE_BD2K8,  // BDS-2 model (not supported currently)
  NEB_IONMODE_BD3GIM, // BDS-3 model (not supported currently)
  NEB_IONMODE_GALNTCM // Galileo ionospheric model
} neb_ionmode_t;

// Wire string for an ionospheric model. Returns NULL for an out-of-range value.
static inline const char *neb_ionmode_str(neb_ionmode_t mode) {
  switch (mode) {
  case NEB_IONMODE_GPSK8:
    return "GPSK8"; // Manual §4.25, Table 4-33
  case NEB_IONMODE_BD2K8:
    return "BD2K8"; // Manual §4.25, Table 4-33
  case NEB_IONMODE_BD3GIM:
    return "BD3GIM"; // Manual §4.25, Table 4-33
  case NEB_IONMODE_GALNTCM:
    return "GALNTCM"; // Manual §4.25, Table 4-33
  default:
    return NULL;
  }
}

// POSITIVE/NEGATIVE polarity, shared by commands that select an edge or sign:
// EVENT trigger edge (Manual §4.11) and PPS pulse polarity (Manual §4.3).
typedef enum {
  NEB_POLARITY_POSITIVE, // rising edge / active high
  NEB_POLARITY_NEGATIVE  // falling edge / active low
} neb_polarity_t;

// Wire string for a polarity. Returns NULL for an out-of-range value.
static inline const char *neb_polarity_str(neb_polarity_t pol) {
  switch (pol) {
  case NEB_POLARITY_POSITIVE:
    return NEB_TOK_POSITIVE;
  case NEB_POLARITY_NEGATIVE:
    return NEB_TOK_NEGATIVE;
  default:
    return NULL;
  }
}

// PPS enable mode (Manual §4.3, Table 4-5). DISABLE is a separate builder.
// ENABLE2/ENABLE3 are not supported on the UM960L.
typedef enum {
  NEB_PPS_ENABLE,  // output after position fix + PPS converged (default)
  NEB_PPS_ENABLE2, // output once time is valid, within +/-100 ms
  NEB_PPS_ENABLE3  // output after the receiver starts to work
} neb_pps_mode_t;

// Wire string for a PPS mode. Returns NULL for an out-of-range value.
static inline const char *neb_pps_mode_str(neb_pps_mode_t mode) {
  switch (mode) {
  case NEB_PPS_ENABLE:
    return NEB_TOK_ENABLE_UC; // Manual §4.3, Table 4-5
  case NEB_PPS_ENABLE2:
    return "ENABLE2"; // Manual §4.3, Table 4-5
  case NEB_PPS_ENABLE3:
    return "ENABLE3"; // Manual §4.3, Table 4-5
  default:
    return NULL;
  }
}

// SBAS system for CONFIG SBAS ENABLE (Manual §4.10, Table 4-15, Parameter 2).
typedef enum {
  NEB_SBAS_AUTO,  // automatic mode
  NEB_SBAS_WAAS,  // WAAS only
  NEB_SBAS_GAGAN, // GAGAN only
  NEB_SBAS_MSAS,  // MSAS only
  NEB_SBAS_EGNOS, // EGNOS only
  NEB_SBAS_SDCM,  // SDCM only
  NEB_SBAS_BDS    // BDS SBAS only
} neb_sbas_system_t;

// Wire string for an SBAS system. Returns NULL for an out-of-range value.
static inline const char *neb_sbas_system_str(neb_sbas_system_t sys) {
  switch (sys) {
  case NEB_SBAS_AUTO:
    return "Auto"; // Manual §4.10, Table 4-15
  case NEB_SBAS_WAAS:
    return "WAAS"; // Manual §4.10, Table 4-15
  case NEB_SBAS_GAGAN:
    return "GAGAN"; // Manual §4.10, Table 4-15
  case NEB_SBAS_MSAS:
    return "MSAS"; // Manual §4.10, Table 4-15
  case NEB_SBAS_EGNOS:
    return "EGNOS"; // Manual §4.10, Table 4-15
  case NEB_SBAS_SDCM:
    return "SDCM"; // Manual §4.10, Table 4-15
  case NEB_SBAS_BDS:
    return "BDS"; // Manual §4.10, Table 4-15
  default:
    return NULL;
  }
}

// NMEA output messages for the data-output commands (Manual §7.1 standard NMEA
// and §7.2 Unicore-extended slave-antenna variants). This names the messages;
// per-message applicability is still the device's to enforce -- the "...H"
// slave-antenna variants need a dual-antenna receiver (UM982), and some
// messages depend on the configured NMEA version (see CONFIG NMEA0183).
typedef enum {
  // §7.1 standard NMEA (GP-prefixed on input)
  NEB_NMEA_GPDTM, // datum reference
  NEB_NMEA_GPGBS, // satellite fault detection
  NEB_NMEA_GPGGA, // fix data
  NEB_NMEA_GPGLL, // geographic position
  NEB_NMEA_GPGNS, // GNSS fix data
  NEB_NMEA_GPGRS, // range residuals
  NEB_NMEA_GPGSA, // DOP and active satellites
  NEB_NMEA_GPGST, // pseudorange error statistics
  NEB_NMEA_GPGSV, // satellites in view
  NEB_NMEA_GPRMC, // recommended minimum GNSS data
  NEB_NMEA_GPROT, // rate of turn
  NEB_NMEA_GPTHS, // true heading and status
  NEB_NMEA_GPVTG, // course and ground speed
  NEB_NMEA_GPZDA, // time and date
  // §7.2 Unicore-extended, slave antenna ("...H")
  NEB_NMEA_GPGGAH,
  NEB_NMEA_GPGLLH,
  NEB_NMEA_GPGNSH,
  NEB_NMEA_GPGRSH,
  NEB_NMEA_GPGSAH,
  NEB_NMEA_GPGSTH,
  NEB_NMEA_GPGSVH,
  NEB_NMEA_GPRMCH,
  NEB_NMEA_GPVTGH
} neb_nmea_message_t;

// Wire name for an NMEA message. Returns NULL for an out-of-range value.
static inline const char *neb_nmea_message_str(neb_nmea_message_t msg) {
  switch (msg) {
  case NEB_NMEA_GPDTM:
    return "GPDTM"; // Manual §7.1
  case NEB_NMEA_GPGBS:
    return "GPGBS";
  case NEB_NMEA_GPGGA:
    return "GPGGA";
  case NEB_NMEA_GPGLL:
    return "GPGLL";
  case NEB_NMEA_GPGNS:
    return "GPGNS";
  case NEB_NMEA_GPGRS:
    return "GPGRS";
  case NEB_NMEA_GPGSA:
    return "GPGSA";
  case NEB_NMEA_GPGST:
    return "GPGST";
  case NEB_NMEA_GPGSV:
    return "GPGSV";
  case NEB_NMEA_GPRMC:
    return "GPRMC";
  case NEB_NMEA_GPROT:
    return "GPROT";
  case NEB_NMEA_GPTHS:
    return "GPTHS";
  case NEB_NMEA_GPVTG:
    return "GPVTG";
  case NEB_NMEA_GPZDA:
    return "GPZDA";
  case NEB_NMEA_GPGGAH:
    return "GPGGAH"; // Manual §7.2 (slave antenna)
  case NEB_NMEA_GPGLLH:
    return "GPGLLH";
  case NEB_NMEA_GPGNSH:
    return "GPGNSH";
  case NEB_NMEA_GPGRSH:
    return "GPGRSH";
  case NEB_NMEA_GPGSAH:
    return "GPGSAH";
  case NEB_NMEA_GPGSTH:
    return "GPGSTH";
  case NEB_NMEA_GPGSVH:
    return "GPGSVH";
  case NEB_NMEA_GPRMCH:
    return "GPRMCH";
  case NEB_NMEA_GPVTGH:
    return "GPVTGH";
  default:
    return NULL;
  }
}

// Latitude / longitude hemisphere for $AIDPOS (Manual §6.1, Table 6-1).
typedef enum { NEB_LAT_NORTH, NEB_LAT_SOUTH } neb_lat_dir_t;
typedef enum { NEB_LON_EAST, NEB_LON_WEST } neb_lon_dir_t;

// Wire string for a latitude hemisphere. Returns NULL for out-of-range.
static inline const char *neb_lat_dir_str(neb_lat_dir_t dir) {
  switch (dir) {
  case NEB_LAT_NORTH:
    return "N"; // Manual §6.1, Table 6-1
  case NEB_LAT_SOUTH:
    return "S"; // Manual §6.1, Table 6-1
  default:
    return NULL;
  }
}

// Wire string for a longitude hemisphere. Returns NULL for out-of-range.
static inline const char *neb_lon_dir_str(neb_lon_dir_t dir) {
  switch (dir) {
  case NEB_LON_EAST:
    return "E"; // Manual §6.1, Table 6-1
  case NEB_LON_WEST:
    return "W"; // Manual §6.1, Table 6-1
  default:
    return NULL;
  }
}

// GNSS constellation for MASK/UNMASK (Manual §5, Table 5-4).
typedef enum {
  NEB_GNSS_GPS,
  NEB_GNSS_BDS,
  NEB_GNSS_GLO,
  NEB_GNSS_GAL,
  NEB_GNSS_QZSS,
  NEB_GNSS_IRNSS
} neb_gnss_t;

// Wire string for a GNSS constellation. Returns NULL for an out-of-range value.
static inline const char *neb_gnss_str(neb_gnss_t gnss) {
  switch (gnss) {
  case NEB_GNSS_GPS:
    return "GPS"; // Manual §5, Table 5-4
  case NEB_GNSS_BDS:
    return "BDS"; // Manual §5, Table 5-4
  case NEB_GNSS_GLO:
    return "GLO"; // Manual §5, Table 5-4
  case NEB_GNSS_GAL:
    return "GAL"; // Manual §5, Table 5-4
  case NEB_GNSS_QZSS:
    return "QZSS"; // Manual §5, Table 5-4
  case NEB_GNSS_IRNSS:
    return "IRNSS"; // Manual §5, Table 5-4
  default:
    return NULL;
  }
}

// GNSS signal/frequency for MASK/UNMASK (Manual §5.2, Table 5-4). Grouped by
// constellation. Masking a coarse band (e.g. GPS L1) can disable several
// sub-signals per the manual's notes; that behavior is the device's.
typedef enum {
  // GPS
  NEB_FREQ_L1,
  NEB_FREQ_L1CA,
  NEB_FREQ_L1C,
  NEB_FREQ_L2,
  NEB_FREQ_L2C,
  NEB_FREQ_L2P,
  NEB_FREQ_L5,
  // BDS
  NEB_FREQ_B1,
  NEB_FREQ_B2,
  NEB_FREQ_B3,
  NEB_FREQ_B1I,
  NEB_FREQ_B2I,
  NEB_FREQ_B3I,
  NEB_FREQ_BD3B1C,
  NEB_FREQ_BD3B2A,
  NEB_FREQ_BD3B2B,
  // GLONASS
  NEB_FREQ_R1,
  NEB_FREQ_R2,
  NEB_FREQ_R3,
  // Galileo
  NEB_FREQ_E1,
  NEB_FREQ_E5A,
  NEB_FREQ_E5B,
  NEB_FREQ_E6C,
  // QZSS
  NEB_FREQ_Q1,
  NEB_FREQ_Q2,
  NEB_FREQ_Q5,
  NEB_FREQ_Q1CA,
  NEB_FREQ_Q1C,
  NEB_FREQ_Q2C,
  // IRNSS
  NEB_FREQ_I5
} neb_gnss_freq_t;

// Wire string for a GNSS frequency. Returns NULL for an out-of-range value.
static inline const char *neb_gnss_freq_str(neb_gnss_freq_t freq) {
  switch (freq) {
  case NEB_FREQ_L1:
    return "L1"; // Manual §5.2, Table 5-4
  case NEB_FREQ_L1CA:
    return "L1CA";
  case NEB_FREQ_L1C:
    return "L1C";
  case NEB_FREQ_L2:
    return "L2";
  case NEB_FREQ_L2C:
    return "L2C";
  case NEB_FREQ_L2P:
    return "L2P";
  case NEB_FREQ_L5:
    return "L5";
  case NEB_FREQ_B1:
    return "B1";
  case NEB_FREQ_B2:
    return "B2";
  case NEB_FREQ_B3:
    return "B3";
  case NEB_FREQ_B1I:
    return "B1I";
  case NEB_FREQ_B2I:
    return "B2I";
  case NEB_FREQ_B3I:
    return "B3I";
  case NEB_FREQ_BD3B1C:
    return "BD3B1C";
  case NEB_FREQ_BD3B2A:
    return "BD3B2A";
  case NEB_FREQ_BD3B2B:
    return "BD3B2B";
  case NEB_FREQ_R1:
    return "R1";
  case NEB_FREQ_R2:
    return "R2";
  case NEB_FREQ_R3:
    return "R3";
  case NEB_FREQ_E1:
    return "E1";
  case NEB_FREQ_E5A:
    return "E5a"; // lowercase 'a' per the manual
  case NEB_FREQ_E5B:
    return "E5b"; // lowercase 'b' per the manual
  case NEB_FREQ_E6C:
    return "E6C";
  case NEB_FREQ_Q1:
    return "Q1";
  case NEB_FREQ_Q2:
    return "Q2";
  case NEB_FREQ_Q5:
    return "Q5";
  case NEB_FREQ_Q1CA:
    return "Q1CA";
  case NEB_FREQ_Q1C:
    return "Q1C";
  case NEB_FREQ_Q2C:
    return "Q2C";
  case NEB_FREQ_I5:
    return "I5";
  default:
    return NULL;
  }
}

// Algorithm to reset for CONFIG ALGRESET (Manual §4.24, Table 4-32). Some
// values are model-restricted (RTK2/HEADING require UM982, PPP requires
// UM980/UM982); that gating is enforced in the wrapper.
typedef enum {
  NEB_ALGRESET_RTK1,    // master antenna RTK (all models)
  NEB_ALGRESET_RTK2,    // slave antenna RTK (UM982 only)
  NEB_ALGRESET_HEADING, // heading algorithm (UM982 only)
  NEB_ALGRESET_PPP,     // master antenna PPP (UM980/UM982)
  NEB_ALGRESET_ADR      // ADR, master and slave (all models)
} neb_algreset_type_t;

// Wire string for an ALGRESET type. Returns NULL for an out-of-range value.
static inline const char *neb_algreset_type_str(neb_algreset_type_t type) {
  switch (type) {
  case NEB_ALGRESET_RTK1:
    return "RTK1"; // Manual §4.24, Table 4-32
  case NEB_ALGRESET_RTK2:
    return "RTK2"; // Manual §4.24, Table 4-32
  case NEB_ALGRESET_HEADING:
    return "HEADING"; // Manual §4.24, Table 4-32
  case NEB_ALGRESET_PPP:
    return "PPP"; // Manual §4.24, Table 4-32
  case NEB_ALGRESET_ADR:
    return "ADR"; // Manual §4.24, Table 4-32
  default:
    return NULL;
  }
}

// Base-station antenna type for CONFIG BASEANTENNAMODEL (Manual §4.26).
typedef enum {
  NEB_ANTENNA_NO,  // "NO" (default)
  NEB_ANTENNA_USER // "USER"
} neb_antenna_type_t;

// Wire string for an antenna type. Returns NULL for an out-of-range value.
static inline const char *neb_antenna_type_str(neb_antenna_type_t type) {
  switch (type) {
  case NEB_ANTENNA_NO:
    return "NO"; // Manual §4.26, Table 4-34
  case NEB_ANTENNA_USER:
    return "USER"; // Manual §4.26, Table 4-34
  default:
    return NULL;
  }
}

// PPS time reference (Manual §4.3, Table 4-6, "Timeref").
typedef enum {
  NEB_PPS_TIMEREF_GPS,
  NEB_PPS_TIMEREF_BDS,
  NEB_PPS_TIMEREF_GAL,
  NEB_PPS_TIMEREF_GLO
} neb_pps_timeref_t;

// Wire string for a PPS time reference. Returns NULL for an out-of-range value.
static inline const char *neb_pps_timeref_str(neb_pps_timeref_t ref) {
  switch (ref) {
  case NEB_PPS_TIMEREF_GPS:
    return "GPS"; // Manual §4.3, Table 4-6
  case NEB_PPS_TIMEREF_BDS:
    return "BDS"; // Manual §4.3, Table 4-6
  case NEB_PPS_TIMEREF_GAL:
    return "GAL"; // Manual §4.3, Table 4-6
  case NEB_PPS_TIMEREF_GLO:
    return "GLO"; // Manual §4.3, Table 4-6
  default:
    return NULL;
  }
}

#endif
