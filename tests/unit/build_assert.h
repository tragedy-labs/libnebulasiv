// build_assert.h -- shared helpers for command-builder tests.
//
// BUILD_OK declares a fresh command buffer, runs the builder call (which must
// reference `buf`), and asserts both NEB_OK and the exact expected wire string.
// BUILD_ERR runs the call and asserts the expected error status.
#ifndef NEB_BUILD_ASSERT_H
#define NEB_BUILD_ASSERT_H

#include "unity.h"

#include "neb_core.h"

#define BUILD_OK(call, expected)                                               \
  do {                                                                         \
    char buf[NEB_CMD_BUF_LEN];                                                 \
    TEST_ASSERT_EQUAL_INT(NEB_OK, (call));                                     \
    TEST_ASSERT_EQUAL_STRING((expected), buf);                                 \
  } while (0)

#define BUILD_ERR(call, experr)                                                \
  do {                                                                         \
    char buf[NEB_CMD_BUF_LEN];                                                 \
    TEST_ASSERT_EQUAL_INT((experr), (call));                                   \
  } while (0)

#endif
