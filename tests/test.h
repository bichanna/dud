/*
 * Copyright 2026 Nobuharu Shimazu
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>

typedef struct TestStats {
  int total;
  int passed;
  int failed;
  const char *current_suite;
} TestStats;

static TestStats _test_stats = {0};

#define TEST_COLOR_RED "\033[0;31m"
#define TEST_COLOR_GREEN "\033[0;32m"
#define TEST_COLOR_YELLOW "\033[0;33m"
#define TEST_COLOR_RESET "\033[0m"

#define TEST_SUITE(name)                                                       \
  do {                                                                         \
    _test_stats.current_suite = name;                                          \
    printf("\n" TEST_COLOR_YELLOW "=== Test Suite: %s ===" TEST_COLOR_RESET    \
           "\n",                                                               \
           name);                                                              \
  } while (0)

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    _test_stats.total++;                                                       \
    if (condition) {                                                           \
      _test_stats.passed++;                                                    \
      printf(TEST_COLOR_GREEN "  ✓ " TEST_COLOR_RESET "%s\n", message);        \
    } else {                                                                   \
      _test_stats.failed++;                                                    \
      printf(TEST_COLOR_RED "  ✗ " TEST_COLOR_RESET "%s (at %s:%d)\n",         \
             message, __FILE__, __LINE__);                                     \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_TRUE(expr) ASSERT((expr), #expr " should be true")
#define ASSERT_FALSE(expr) ASSERT(!(expr), #expr " should be false")
#define ASSERT_NULL(ptr) ASSERT((ptr) == NULL, #ptr " should be NULL")
#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != NULL, #ptr " should not be NULL")
#define ASSERT_EQ(a, b) ASSERT((a) == (b), #a " should equal " #b)
#define ASSERT_NEQ(a, b) ASSERT((a) != (b), #a " should not equal " #b)

#define TEST(name)                                                             \
  static bool test_##name(void);                                               \
  static bool test_##name(void)

#define RUN_TEST(name)                                                         \
  do {                                                                         \
    printf("\n" TEST_COLOR_YELLOW "Running: " TEST_COLOR_RESET #name "\n");    \
    if (test_##name()) {                                                       \
      printf(TEST_COLOR_GREEN "PASSED: " TEST_COLOR_RESET #name "\n");         \
    } else {                                                                   \
      printf(TEST_COLOR_RED "FAILED: " TEST_COLOR_RESET #name "\n");           \
    }                                                                          \
  } while (0)

#define TEST_SUMMARY()                                                         \
  do {                                                                         \
    printf("\n" TEST_COLOR_YELLOW                                              \
           "================================" TEST_COLOR_RESET "\n");          \
    printf(TEST_COLOR_YELLOW "Test Summary" TEST_COLOR_RESET "\n");            \
    printf(TEST_COLOR_YELLOW                                                   \
           "================================" TEST_COLOR_RESET "\n");          \
    printf("Total:  %d\n", _test_stats.total);                                 \
    printf(TEST_COLOR_GREEN "Passed: %d\n" TEST_COLOR_RESET,                   \
           _test_stats.passed);                                                \
    if (_test_stats.failed > 0) {                                              \
      printf(TEST_COLOR_RED "Failed: %d\n" TEST_COLOR_RESET,                   \
             _test_stats.failed);                                              \
    } else {                                                                   \
      printf("Failed: 0\n");                                                   \
    }                                                                          \
    printf(TEST_COLOR_YELLOW                                                   \
           "================================" TEST_COLOR_RESET "\n");          \
    if (_test_stats.failed == 0 && _test_stats.total > 0) {                    \
      printf(TEST_COLOR_GREEN "\n✓ All tests passed!\n" TEST_COLOR_RESET);     \
    }                                                                          \
  } while (0)

#define TEST_EXIT_CODE() (_test_stats.failed == 0 ? 0 : 1)
