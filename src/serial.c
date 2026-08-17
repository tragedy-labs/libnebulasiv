#define _DEFAULT_SOURCE

#include "serial.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>

static speed_t baudrate_to_termios_speed(int baudrate) {
  switch (baudrate) {
  case 9600:
    return B9600;
  case 19200:
    return B19200;
  case 38400:
    return B38400;
  case 57600:
    return B57600;
  case 115200:
    return B115200;
  case 230400:
    return B230400;
  case 460800:
    return B460800;
  case 921600:
    return B921600;
  default:
    return 0;
  }
}

int serial_open(serial_t *serial, const char *device, int baudrate) {
  if (!serial || !device)
    return -1;

  speed_t speed = baudrate_to_termios_speed(baudrate);

  if (speed == 0)
    return -1;

  int fd = open(device, O_RDWR | O_NOCTTY);

  if (fd < 0)
    return -1;

  struct termios tty;

  if (tcgetattr(fd, &tty) != 0) {
    close(fd);
    return -1;
  }

  cfmakeraw(&tty);

  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  tty.c_cflag |= CLOCAL | CREAD;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;

  // Fully non-blocking at the termios level; bounded waiting is done in
  // serial_read_timed() via poll(), which is cleaner than juggling VTIME.
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    close(fd);
    return -1;
  }

  serial->fd = fd;

  return 0;
}

void serial_close(serial_t *serial) {
  if (!serial)
    return;

  if (serial->fd >= 0) {
    close(serial->fd);
    serial->fd = -1;
  }
}

ssize_t serial_read(serial_t *serial, uint8_t *buffer, size_t length) {
  if (!serial || serial->fd < 0 || !buffer || length == 0)
    return -1;

  ssize_t n = read(serial->fd, buffer, length);

  // A non-blocking read with nothing available reports EAGAIN; treat that as
  // "no data" (0) rather than an error so callers can poll cleanly.
  if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    return 0;

  return n;
}

ssize_t serial_read_timed(serial_t *serial, uint8_t *buffer, size_t length,
                          int timeout_ms) {
  if (!serial || serial->fd < 0 || !buffer || length == 0)
    return -1;

  struct pollfd pfd = {.fd = serial->fd, .events = POLLIN};

  for (;;) {
    int rc = poll(&pfd, 1, timeout_ms);

    if (rc < 0) {
      if (errno == EINTR)
        continue; // interrupted before the timeout; retry
      return -1;
    }

    if (rc == 0)
      return 0; // timed out with no data

    return serial_read(serial, buffer, length);
  }
}

ssize_t serial_write(serial_t *serial, const uint8_t *buffer, size_t length) {
  if (!serial || serial->fd < 0 || !buffer || length == 0)
    return -1;

  size_t written = 0;

  while (written < length) {
    ssize_t n = write(serial->fd, buffer + written, length - written);

    if (n < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }

    written += (size_t)n;
  }

  return (ssize_t)written;
}

void serial_flush_input(serial_t *serial) {
  if (!serial || serial->fd < 0)
    return;

  tcflush(serial->fd, TCIFLUSH);
}
