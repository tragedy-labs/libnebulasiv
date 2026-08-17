// tests.h -- per-file test-group entry points, invoked by test_main.c between
// UNITY_BEGIN() and UNITY_END().
#ifndef NEB_TESTS_H
#define NEB_TESTS_H

void run_mode_build_tests(void);
void run_config_build_tests(void);
void run_rtk_build_tests(void);
void run_mask_build_tests(void);
void run_assist_build_tests(void);
void run_heading_build_tests(void);
void run_admin_build_tests(void);
void run_logging_build_tests(void);
void run_send_command_tests(void);
void run_caps_tests(void);

#endif
