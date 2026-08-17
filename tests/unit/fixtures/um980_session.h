// um980_session.h
//
// Real device responses captured from an actual UM980 (R4.10Build13504) over
// /dev/ttyUSB0. These are the canned bytes replayed through the mock transport
// in neb_send_command() tests. They are observations, not invented values --
// the acknowledgement format is not documented in the manual, so it is pinned
// here from hardware.
//
// Payload checksums (e.g. the "#MODE,...*15") vary per reading because they
// cover a timestamp field; the ack checksum (e.g. "OK*5D") is stable. Parser
// tests do not depend on payload checksums.
#ifndef NEB_FIX_UM980_SESSION_H
#define NEB_FIX_UM980_SESSION_H

// MODE query: ack line followed by the "#MODE,...;MODE ROVER SURVEY,*hh"
// record.
#define UM980_FIX_MODE_QUERY_OK                                                \
  "$command,MODE,response: OK*5D\r\n"                                          \
  "#MODE,97,GPS,FINE,2431,596371000,0,0,18,120;MODE ROVER SURVEY,*15\r\n"

// CONFIG query: ack line followed by "$CONFIG,..." records (abridged).
#define UM980_FIX_CONFIG_QUERY_OK                                              \
  "$command,CONFIG,response: OK*54\r\n"                                        \
  "$CONFIG,ANTENNA,CONFIG ANTENNA POWERON*7A\r\n"                              \
  "$CONFIG,COM1,CONFIG COM1 115200*23\r\n"

// Rejection: a well-formed header with a bad parameter.
#define UM980_FIX_NAK_MISSING_FIELD                                            \
  "$command,config com9 115200,response: PARSING FAILED MISSING FIELD,*56\r\n"

// Rejection: an unknown command header.
#define UM980_FIX_NAK_NO_FUNC                                                  \
  "$command,THISHEADERDOESNOTEXIST,response: PARSING FAILED NO MATCHING FUNC " \
  " THISHEADERDOESNOTEXIST*05\r\n"

// Successful ack preceded by stray bytes (observed leading the first reply).
#define UM980_FIX_OK_WITH_GARBAGE_PREFIX                                       \
  "\xcc\xfd"                                                                   \
  "$command,unlog,response: OK*21\r\n"

#endif
