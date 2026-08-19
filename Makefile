# Test Makefile for libnebulasiv.
#
#   make            / make test           -> unit tests only (CI-safe, no HW)
#   make test-hardware                    -> integration tests against whatever
#                                            receiver is at $NEB_TEST_PORT, and
#                                            regenerate the hardware matrix
#   make hardware-matrix                  -> regenerate the matrix only, from
#                                            the recorded results (no hardware)
#   make test-all                         -> both
#   make clean
#
# The unit target compiles our code strictly (-Wall -Wextra -Wpedantic -Werror)
# under AddressSanitizer + UndefinedBehaviorSanitizer, so buffer overruns in the
# fixed-size command buffers are caught at run time. Vendored Unity is compiled
# with relaxed flags so third-party source can't break our build.

CC      ?= cc
INCLUDES := -Iinclude -Itests/unity -Itests/unit -Itests/integration
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

# One hardware suite for every model. It identifies the attached receiver with
# VERSIONA and decides per test what that model and firmware build should do,
# rather than a separate file per chip -- see tests/integration/test_hil.c.
HIL_SRC := \
  tests/integration/hil_device.c \
  tests/integration/test_hil.c

RESULTS_DIR := tests/results
MATRIX_DOC  := HARDWARE_TESTING.md
MATRIX_AWK  := tools/gen_hardware_matrix.awk

# Recorded automatically so a contributed run is attributable; override freely.
NEB_TEST_CONTRIBUTOR ?= $(shell git config user.name 2>/dev/null)
export NEB_TEST_CONTRIBUTOR

.PHONY: test test-unit test-hardware hardware-matrix test-all clean

test: test-unit

test-unit: $(BUILD)/unit_tests
	$(BUILD)/unit_tests

$(BUILD)/unit_tests: $(LIB_SRC) $(UNIT_SRC) tests/unity/unity.c
	@mkdir -p $(BUILD)
	$(CC) $(DBG) $(SAN) -Itests/unity -c tests/unity/unity.c -o $(BUILD)/unity.o
	$(CC) $(STRICT) $(DBG) $(SAN) $(INCLUDES) $(LIB_SRC) $(UNIT_SRC) \
	    $(BUILD)/unity.o -o $@

# Runs against whatever is attached: the suite identifies the receiver and
# adapts. RAM-only by default (never SAVECONFIG); NEB_TEST_LEVEL=read restricts
# it to read-only queries. NEB_TEST_BOARD names the carrier board, which no
# query can reveal.
test-hardware: $(BUILD)/hil_tests
	@if [ -z "$(NEB_TEST_PORT)" ]; then \
	    echo "NEB_TEST_PORT is not set (e.g. NEB_TEST_PORT=/dev/ttyUSB0)."; \
	    echo "Refusing to run hardware tests without an explicit port."; \
	    exit 2; \
	fi
	@mkdir -p $(RESULTS_DIR)
	$(BUILD)/hil_tests
	@$(MAKE) --no-print-directory hardware-matrix

$(BUILD)/hil_tests: $(LIB_SRC) $(HIL_SRC) tests/unity/unity.c
	@mkdir -p $(BUILD)
	$(CC) $(DBG) $(SAN) -Itests/unity -c tests/unity/unity.c -o $(BUILD)/unity.o
	$(CC) $(STRICT) $(DBG) $(SAN) $(INCLUDES) $(LIB_SRC) $(HIL_SRC) \
	    $(BUILD)/unity.o -o $@

# Rebuild the matrix in HARDWARE_TESTING.md from the recorded runs. Needs no
# hardware -- the result files are the record; the table is only a view.
hardware-matrix:
	@files=$$(ls $(RESULTS_DIR)/*.tsv 2>/dev/null); \
	awk -f $(MATRIX_AWK) $$files $(MATRIX_DOC) > $(MATRIX_DOC).tmp \
	    && mv $(MATRIX_DOC).tmp $(MATRIX_DOC) \
	    && echo "Regenerated the matrix in $(MATRIX_DOC)."

test-all: test-unit test-hardware

clean:
	rm -rf $(BUILD)
