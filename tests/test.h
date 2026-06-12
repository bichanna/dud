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

/*
 * Output is compact by default (Cargo-style): the run prints one check mark per
 * test and nothing else -- a passing test is a green mark, a failing test a red
 * mark, packed onto a line per suite. Every failure's detail (the assertion and
 * anything the tested code wrote to stdout/stderr) is collected into a single
 * "Failed tests:" section at the end, so the live output stays a clean stream
 * of marks. Call TEST_VERBOSE(1) for full line-per-assertion output instead.
 *
 * Color: ANSI escapes, ON by default. Disable with the NO_COLOR environment
 * variable or by defining TEST_NO_COLOR. (No terminal detection -- that breaks
 * under test harnesses like `meson test` that capture stdout.)
 *
 * POSIX: the only non-standard-C piece is per-test stdout/stderr capture, which
 * needs dup()/dup2() to restore them afterwards. Define TEST_NO_STDERR_CAPTURE
 * for a pure C17 build with no POSIX dependency (capture becomes a no-op).
 */

#ifndef TEST_NO_STDERR_CAPTURE
// POSIX dup/dup2/fileno under -std=c17. Must precede any system header.
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TEST_NO_STDERR_CAPTURE
#include <unistd.h>
#endif

#ifndef TEST_MAX_FAILURES
#define TEST_MAX_FAILURES 256
#endif

#define TEST_COLOR_RED "\033[0;31m"
#define TEST_COLOR_GREEN "\033[0;32m"
#define TEST_COLOR_YELLOW "\033[0;33m"
#define TEST_COLOR_RESET "\033[0m"

#define TEST_MARK_PASS "\u2713"
#define TEST_MARK_FAIL "\u2717"

typedef struct TestFailure {
  const char *suite;
  const char *test;
  const char *msg;
  const char *file;
  int line;
  char *output_text; // captured stdout + stderr (compact mode only), or NULL
} TestFailure;

typedef struct TestStats {
  int total;
  int passed;
  int failed;
  int tests_run;
  int verbose;             // 0 = compact (default), 1 = line-per-assertion
  int verbose_initialized; // 1 = checked env vars
  int color_state;         // 0 = unresolved, 1 = off, 2 = on
  int pending_marks;       // 1 = compact check marks are waiting for a newline
  const char *current_suite;
  const char *current_test;
  FILE *framework_out;
  TestFailure failures[TEST_MAX_FAILURES];
} TestStats;

static TestStats _test_stats = {0};

static inline void _test_init(void) {
  if (_test_stats.framework_out == NULL) {
#ifndef TEST_NO_STDERR_CAPTURE
    int fd = dup(fileno(stdout));
    if (fd >= 0) {
      _test_stats.framework_out = fdopen(fd, "w");
    }
#endif
    if (_test_stats.framework_out == NULL) {
      _test_stats.framework_out = stdout;
    }
  }
}

static inline void _tprintf(const char *fmt, ...) {
  _test_init();
  va_list args;
  va_start(args, fmt);
  vfprintf(_test_stats.framework_out, fmt, args);
  va_end(args);
}

static inline int _test_is_verbose(void) {
  if (!_test_stats.verbose_initialized) {
    const char *env = getenv("TEST_VERBOSE");
    _test_stats.verbose = (env && env[0] == '1') ? 1 : 0;
    _test_stats.verbose_initialized = 1;
  }
  return _test_stats.verbose;
}

static inline int _test_color_enabled(void) {
  if (_test_stats.color_state == 0) {
    int on = 1;
#ifdef TEST_NO_COLOR
    on = 0;
#else
    if (getenv("NO_COLOR") != NULL) {
      on = 0;
    }
#endif
    _test_stats.color_state = on ? 2 : 1;
  }
  return _test_stats.color_state == 2;
}

static inline const char *_tc(const char *code) {
  return _test_color_enabled() ? code : "";
}

static inline void _test_cprint(const char *color, const char *text) {
  _tprintf("%s%s%s", _tc(color), text, _tc(TEST_COLOR_RESET));
}

static inline void _test_mark(bool pass) {
  _test_cprint(pass ? TEST_COLOR_GREEN : TEST_COLOR_RED,
               pass ? TEST_MARK_PASS : TEST_MARK_FAIL);
}

static inline void _test_rule(void) {
  _test_cprint(TEST_COLOR_YELLOW, "================================");
  _tprintf("\n");
}

static inline void _test_flush_marks(void) {
  if (_test_stats.pending_marks) {
    _tprintf("\n");
    _test_stats.pending_marks = 0;
  }
}

static inline int _test_streq(const char *a, const char *b) {
  if (a == b) {
    return 1;
  }
  if (a == NULL || b == NULL) {
    return 0;
  }
  return strcmp(a, b) == 0;
}

#ifndef TEST_NO_STDERR_CAPTURE

typedef struct TestCapture {
  int saved_stdout;
  int saved_stderr;
  FILE *tmp;
} TestCapture;

static inline void _test_capture_begin(TestCapture *c) {
  _test_init();
  c->saved_stdout = -1;
  c->saved_stderr = -1;
  c->tmp = tmpfile();
  if (c->tmp == NULL) {
    return; // capture unavailable; tests still run normally
  }
  fflush(stdout);
  fflush(stderr);

  int s_out = dup(fileno(stdout));
  int s_err = dup(fileno(stderr));
  if (s_out < 0 || s_err < 0) {
    if (s_out >= 0)
      close(s_out);
    if (s_err >= 0)
      close(s_err);
    fclose(c->tmp);
    c->tmp = NULL;
    return;
  }
  c->saved_stdout = s_out;
  c->saved_stderr = s_err;

  // Reroute both streams into the identical tmp file to correctly interleave
  // output
  dup2(fileno(c->tmp), fileno(stdout));
  dup2(fileno(c->tmp), fileno(stderr));
}

static inline void _test_capture_end(TestCapture *c) {
  if (c->saved_stdout < 0) {
    return;
  }
  fflush(stdout);
  fflush(stderr);
  dup2(c->saved_stdout, fileno(stdout));
  dup2(c->saved_stderr, fileno(stderr));
  close(c->saved_stdout);
  close(c->saved_stderr);
  c->saved_stdout = -1;
  c->saved_stderr = -1;
}

static inline char *_test_capture_take(TestCapture *c) {
  if (c->tmp == NULL) {
    return NULL;
  }
  if (fseek(c->tmp, 0, SEEK_END) != 0) {
    return NULL;
  }
  long sz = ftell(c->tmp);
  if (sz <= 0) {
    return NULL;
  }
  rewind(c->tmp);
  char *buf = malloc((size_t)sz + 1);
  if (buf == NULL) {
    return NULL;
  }
  size_t n = fread(buf, 1, (size_t)sz, c->tmp);
  buf[n] = '\0';
  return buf;
}

static inline void _test_capture_dump(TestCapture *c) {
  if (c->tmp == NULL) {
    return;
  }
  rewind(c->tmp);
  char line[512];
  int header = 0;
  while (fgets(line, (int)sizeof(line), c->tmp) != NULL) {
    if (!header) {
      _tprintf("      ");
      _test_cprint(TEST_COLOR_YELLOW, "--- output ---");
      _tprintf("\n");
      header = 1;
    }
    _tprintf("      | %s", line);
    size_t len = strlen(line);
    if (len == 0 || line[len - 1] != '\n') {
      _tprintf("\n");
    }
  }
}

static inline void _test_capture_close(TestCapture *c) {
  if (c->tmp != NULL) {
    fclose(c->tmp);
    c->tmp = NULL;
  }
}

#else

typedef struct TestCapture {
  int unused;
} TestCapture;

static inline void _test_capture_begin(TestCapture *c) { (void)c; }
static inline void _test_capture_end(TestCapture *c) { (void)c; }
static inline char *_test_capture_take(TestCapture *c) {
  (void)c;
  return NULL;
}
static inline void _test_capture_dump(TestCapture *c) { (void)c; }
static inline void _test_capture_close(TestCapture *c) { (void)c; }

#endif

static inline void _test_print_output_text(const char *text) {
  if (text == NULL || text[0] == '\0') {
    return;
  }
  _tprintf("      ");
  _test_cprint(TEST_COLOR_YELLOW, "--- output ---");
  _tprintf("\n");
  const char *p = text;
  while (*p != '\0') {
    const char *nl = strchr(p, '\n');
    _tprintf("      | ");
    if (nl != NULL) {
      _tprintf("%.*s\n", (int)(nl - p), p);
      p = nl + 1;
    } else {
      _tprintf("%s\n", p);
      break;
    }
  }
}

static inline void _test_suite(const char *name) {
  _test_init();
  _test_flush_marks();
  _test_stats.current_suite = name;
  if (_test_is_verbose()) {
    _tprintf("\n%s=== Test Suite: %s ===%s\n", _tc(TEST_COLOR_YELLOW), name,
             _tc(TEST_COLOR_RESET));
  } else {
    _tprintf("%s%s:%s ", _tc(TEST_COLOR_YELLOW), name, _tc(TEST_COLOR_RESET));
  }
}

static inline void _test_record_failure(const char *msg, const char *file,
                                        int line) {
  _test_stats.failed++;
  if (_test_stats.failed <= TEST_MAX_FAILURES) {
    TestFailure *f = &_test_stats.failures[_test_stats.failed - 1];
    f->suite = _test_stats.current_suite;
    f->test = _test_stats.current_test;
    f->msg = msg;
    f->file = file;
    f->line = line;
    f->output_text = NULL;
  }
}

static inline void _test_assert_pass(const char *msg) {
  _test_stats.passed++;
  if (_test_is_verbose()) {
    _tprintf("  ");
    _test_mark(true);
    _tprintf(" %s\n", msg);
  }
}

static inline void _test_assert_fail(const char *msg, const char *file,
                                     int line) {
  _test_record_failure(msg, file, line);
  if (_test_is_verbose()) {
    _tprintf("  ");
    _test_mark(false);
    _tprintf(" %s (at %s:%d)\n", msg, file, line);
  }
}

static inline void _test_run_begin(const char *name) {
  _test_init();
  _test_stats.current_test = name;
  _test_stats.tests_run++;
  if (_test_is_verbose()) {
    _tprintf("\n%sRunning: %s%s\n", _tc(TEST_COLOR_YELLOW),
             _tc(TEST_COLOR_RESET), name);
  }
}

static inline void _test_run_end(const char *name, bool ok, int failed_before,
                                 TestCapture *cap) {
  if (_test_is_verbose()) {
    if (ok) {
      _tprintf("%sPASSED: %s%s\n", _tc(TEST_COLOR_GREEN), _tc(TEST_COLOR_RESET),
               name);
    } else {
      _tprintf("%sFAILED: %s%s\n", _tc(TEST_COLOR_RED), _tc(TEST_COLOR_RESET),
               name);
      _test_capture_dump(cap); // detail shown inline in verbose
    }
    return;
  }

  // compact: just a packed mark, detail is deferred to the summary
  _test_mark(ok);
  _test_stats.pending_marks = 1;
  fflush(_test_stats.framework_out);
  if (!ok && _test_stats.failed > failed_before &&
      _test_stats.failed <= TEST_MAX_FAILURES) {
    _test_stats.failures[_test_stats.failed - 1].output_text =
        _test_capture_take(cap);
  }
}

static inline void _test_summary(void) {
  _test_flush_marks();
  _tprintf("\n");
  _test_rule();
  _test_cprint(TEST_COLOR_YELLOW, "Test Summary");
  _tprintf("\n");
  _test_rule();
  _tprintf("Total:  %d\n", _test_stats.total);
  _tprintf("%sPassed: %d%s\n", _tc(TEST_COLOR_GREEN), _test_stats.passed,
           _tc(TEST_COLOR_RESET));
  if (_test_stats.failed > 0) {
    _tprintf("%sFailed: %d%s\n", _tc(TEST_COLOR_RED), _test_stats.failed,
             _tc(TEST_COLOR_RESET));
  } else {
    _tprintf("Failed: 0\n");
  }
  _test_rule();

  if (_test_stats.failed > 0) {
    int shown = _test_stats.failed < TEST_MAX_FAILURES ? _test_stats.failed
                                                       : TEST_MAX_FAILURES;
    _tprintf("\n");
    _test_cprint(TEST_COLOR_RED, "Failed tests:");
    _tprintf("\n");
    for (int i = 0; i < shown; i++) {
      TestFailure *f = &_test_stats.failures[i];
      _tprintf("  ");
      _test_mark(false);
      _tprintf(" %s / %s\n", f->suite ? f->suite : "(no suite)",
               f->test ? f->test : "(unknown)");
      _tprintf("      %s (at %s:%d)\n", f->msg ? f->msg : "", f->file, f->line);
      _test_print_output_text(f->output_text);
      free(f->output_text);
      f->output_text = NULL;
    }
    if (_test_stats.failed > TEST_MAX_FAILURES) {
      _tprintf("  ... and %d more\n", _test_stats.failed - TEST_MAX_FAILURES);
    }
  } else if (_test_stats.total > 0) {
    _tprintf("\n");
    _test_mark(true);
    _test_cprint(TEST_COLOR_GREEN, " All tests passed!");
    _tprintf("\n");
  }
}

/* --- public macros ------------------------------------------------------ */

#define TEST_SUITE(name) _test_suite((name))

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    _test_stats.total++;                                                       \
    if (condition) {                                                           \
      _test_assert_pass((message));                                            \
    } else {                                                                   \
      _test_assert_fail((message), __FILE__, __LINE__);                        \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_TRUE(expr) ASSERT((expr), #expr " should be true")
#define ASSERT_FALSE(expr) ASSERT(!(expr), #expr " should be false")
#define ASSERT_NULL(ptr) ASSERT((ptr) == NULL, #ptr " should be NULL")
#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != NULL, #ptr " should not be NULL")
#define ASSERT_EQ(a, b) ASSERT((a) == (b), #a " should equal " #b)
#define ASSERT_NEQ(a, b) ASSERT((a) != (b), #a " should not equal " #b)
#define ASSERT_STR_EQ(a, b)                                                    \
  ASSERT(_test_streq((a), (b)), #a " should equal " #b " (string)")
#define ASSERT_STR_NEQ(a, b)                                                   \
  ASSERT(!_test_streq((a), (b)), #a " should not equal " #b " (string)")

#define TEST(name)                                                             \
  static bool test_##name(void);                                               \
  static bool test_##name(void)

#define RUN_TEST(name)                                                         \
  do {                                                                         \
    TestCapture _cap;                                                          \
    int _before = _test_stats.failed;                                          \
    _test_run_begin(#name);                                                    \
    _test_capture_begin(&_cap);                                                \
    bool _ok = test_##name();                                                  \
    _test_capture_end(&_cap);                                                  \
    _test_run_end(#name, _ok, _before, &_cap);                                 \
    _test_capture_close(&_cap);                                                \
  } while (0)

#define TEST_SUMMARY() _test_summary()

#define TEST_EXIT_CODE() (_test_stats.failed == 0 ? 0 : 1)

// Toggle full per-assertion output at runtime: TEST_VERBOSE(1) on,
// TEST_VERBOSE(0) off.
#define TEST_VERBOSE(on)                                                       \
  do {                                                                         \
    _test_stats.verbose = (on);                                                \
    _test_stats.verbose_initialized = 1;                                       \
  } while (0)
