// neb_internal.h
//
// Library-private helpers shared across the src/ modules. This header is NOT
// part of the public API -- it lives under src/, is not installed, and no
// bundle header includes it. Consumers never see these symbols.
#ifndef NEB_INTERNAL_H
#define NEB_INTERNAL_H

#include <stddef.h>
#include <string.h>

// Validate a caller-supplied free-text token destined for the wire: 1..31
// printable ASCII characters (0x21..0x7e), never a double-quote (which would
// break the command's wire quoting), and -- unless `allow_space` is nonzero --
// never a space (so the value stays a single token). This rejects control
// characters, CR/LF and non-ASCII bytes, guarding against token / line-break /
// quote injection through names, serial numbers and message identifiers.
//
// `allow_space` is nonzero only for fields that are quoted on the wire (e.g. a
// base-antenna name); unquoted fields (serial numbers, log-message names) pass
// 0. Returns 1 if valid, 0 otherwise (including a NULL pointer).
static inline int neb_valid_token(const char *s, int allow_space) {
  if (!s)
    return 0;
  size_t n = strlen(s);
  if (n < 1 || n > 31)
    return 0;
  for (size_t i = 0; i < n; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c < 0x20 || c > 0x7e) // control chars (incl. CR/LF) and non-ASCII
      return 0;
    if (c == '"')
      return 0;
    if (!allow_space && c == ' ')
      return 0;
  }
  return 1;
}

#endif
