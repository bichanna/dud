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

#include "src/allocator.h"
#include "test.h"

TEST(raw_allocator_alloc) {
  int64_t *ptr = ALLOC(&raw_allocator, sizeof(int64_t), "test");
  ASSERT_NOT_NULL(ptr);
  FREE(&raw_allocator, ptr, 1, "test");
  return true;
}

TEST(raw_allocator_realloc) {
  int64_t *ptr = ALLOC(&raw_allocator, sizeof(int64_t), "test");
  ASSERT_NOT_NULL(ptr);

  int64_t *new_ptr = REALLOC(&raw_allocator, ptr, sizeof(int64_t),
                             sizeof(int64_t) * 3, "test");
  ASSERT_NOT_NULL(new_ptr);

  FREE(&raw_allocator, new_ptr, sizeof(int64_t) * 3, "test");
  return true;
}

TEST(tracing_allocator) {
  LogSink sink = {console_log, NULL};
  TracingContext ctx = {0};
  ctx.sink = &sink;

  Allocator tracing_allocator = {tracing_alloc, tracing_realloc, tracing_free,
                                 &ctx};
  ALLOC(&tracing_allocator, sizeof(int64_t), "integer");
  char *ptr = ALLOC(&tracing_allocator, sizeof(char) * 11, "string");
  FREE(&tracing_allocator, ptr, sizeof(char) * 11, "string");

  dump_memory_leaks(&ctx);
  ASSERT_EQ(ctx.freed, 11);
  ASSERT_EQ(ctx.allocated, sizeof(int64_t) + sizeof(char) * 11);

  return true;
}

int main(void) {
  TEST_SUITE("Allocator Tests");
  RUN_TEST(raw_allocator_alloc);
  RUN_TEST(raw_allocator_realloc);
  RUN_TEST(tracing_allocator);

  TEST_SUMMARY();
  return TEST_EXIT_CODE();
}
