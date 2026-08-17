// test_main.c -- hand-written Unity runner. setUp/tearDown are defined once
// here for all test groups; each run_*_tests() invokes RUN_TEST per test.
#include "unity.h"

#include "tests.h"

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();
  run_mode_build_tests();
  run_config_build_tests();
  run_rtk_build_tests();
  run_mask_build_tests();
  run_assist_build_tests();
  run_heading_build_tests();
  run_admin_build_tests();
  run_logging_build_tests();
  run_send_command_tests();
  run_caps_tests();
  return UNITY_END();
}
