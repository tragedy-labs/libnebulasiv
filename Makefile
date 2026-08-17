# Test Makefile for libnebulasiv.
#
#   make            / make test           -> unit tests only (CI-safe, no HW)
#   make test-hardware                    -> integration tests, needs a real
#                                            UM980 at $NEB_TEST_PORT (opt-in)
#   make test-all                         -> both
#   make clean
#
# The unit target compiles our code strictly (-Wall -Wextra -Wpedantic -Werror)
# under AddressSanitizer + UndefinedBehaviorSanitizer, so buffer overruns in the
# fixed-size command buffers are caught at run time. Vendored Unity is compiled
# with relaxed flags so third-party source can't break our build.

CC      ?= cc
INCLUDES := -Iinclude -Itests/unity -Itests/unit
STRICT  := -std=gnu11 -Wall -Wextra -Wpedantic -Werror
SAN     := -fsanitize=address,undefined -fno-omit-frame-pointer
DBG     := -g -O1
BUILD   := build/tests

# Production sources exercised by the tests (serial/transport are linked so
# neb_open resolves, though unit tests drive the mock transport instead).
LIB_SRC := \
  src/serial.c \
  src/neb_serial_transport.c \
  src/neb_core.c \
  src/neb_mode.c \
  src/neb_config.c \
  src/neb_rtk.c \
  src/neb_mask.c \
  src/neb_assist.c \
  src/neb_heading.c \
  src/neb_admin.c \
  src/neb_logging.c

UNIT_SRC := \
  tests/unit/mock_transport.c \
  tests/unit/test_neb_mode_build.c \
  tests/unit/test_neb_config_build.c \
  tests/unit/test_neb_rtk_build.c \
  tests/unit/test_neb_mask_build.c \
  tests/unit/test_neb_assist_build.c \
  tests/unit/test_neb_heading_build.c \
  tests/unit/test_neb_admin_build.c \
  tests/unit/test_neb_logging_build.c \
  tests/unit/test_neb_send_command.c \
  tests/unit/test_neb_caps.c \
  tests/unit/test_bundle_um960.c \
  tests/unit/test_bundle_um960l.c \
  tests/unit/test_bundle_um980.c \
  tests/unit/test_bundle_um982.c \
  tests/unit/test_main.c

HIL_SRC := tests/integration/test_hil_um980.c
HIL_UM982_SRC := tests/integration/test_hil_um982.c

.PHONY: test test-unit test-hardware test-hardware-um982 test-all clean

test: test-unit

test-unit: $(BUILD)/unit_tests
	$(BUILD)/unit_tests

$(BUILD)/unit_tests: $(LIB_SRC) $(UNIT_SRC) tests/unity/unity.c
	@mkdir -p $(BUILD)
	$(CC) $(DBG) $(SAN) -Itests/unity -c tests/unity/unity.c -o $(BUILD)/unity.o
	$(CC) $(STRICT) $(DBG) $(SAN) $(INCLUDES) $(LIB_SRC) $(UNIT_SRC) \
	    $(BUILD)/unity.o -o $@

test-hardware: $(BUILD)/hil_tests
	@if [ -z "$(NEB_TEST_PORT)" ]; then \
	    echo "NEB_TEST_PORT is not set (e.g. NEB_TEST_PORT=/dev/ttyUSB0)."; \
	    echo "Refusing to run hardware tests without an explicit port."; \
	    exit 2; \
	fi
	$(BUILD)/hil_tests

$(BUILD)/hil_tests: $(LIB_SRC) $(HIL_SRC) tests/unity/unity.c
	@mkdir -p $(BUILD)
	$(CC) $(DBG) $(SAN) -Itests/unity -c tests/unity/unity.c -o $(BUILD)/unity.o
	$(CC) $(STRICT) $(DBG) $(SAN) $(INCLUDES) $(LIB_SRC) $(HIL_SRC) \
	    $(BUILD)/unity.o -o $@

# UM982-only hardware verification (Holybro H-RTK Unicore UM982). RAM-only;
# never persists. Needs a real UM982 at $NEB_TEST_PORT.
test-hardware-um982: $(BUILD)/hil_um982
	@if [ -z "$(NEB_TEST_PORT)" ]; then \
	    echo "NEB_TEST_PORT is not set (e.g. NEB_TEST_PORT=/dev/ttyUSB0)."; \
	    echo "Refusing to run hardware tests without an explicit port."; \
	    exit 2; \
	fi
	$(BUILD)/hil_um982

$(BUILD)/hil_um982: $(LIB_SRC) $(HIL_UM982_SRC) tests/unity/unity.c
	@mkdir -p $(BUILD)
	$(CC) $(DBG) $(SAN) -Itests/unity -c tests/unity/unity.c -o $(BUILD)/unity.o
	$(CC) $(STRICT) $(DBG) $(SAN) $(INCLUDES) $(LIB_SRC) $(HIL_UM982_SRC) \
	    $(BUILD)/unity.o -o $@

test-all: test-unit test-hardware

clean:
	rm -rf $(BUILD)
