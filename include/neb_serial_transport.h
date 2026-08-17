// neb_serial_transport.h
//
// The production implementation of neb_transport_t, backed by the termios
// serial layer. neb_open() uses this to wire a handle to a real port; tests
// substitute their own neb_transport_t instead.
#ifndef NEB_SERIAL_TRANSPORT_H
#define NEB_SERIAL_TRANSPORT_H

#include "neb_core.h"
#include "serial.h"

// Build a transport that reads/writes through `serial`, which must outlive the
// transport (neb_open backs it with the handle's own serial_t).
neb_transport_t neb_serial_transport(serial_t *serial);

#endif
