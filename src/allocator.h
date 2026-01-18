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

#include <stdarg.h>
#include <stddef.h>

typedef struct Allocator {
  void *(*alloc)(void *context, size_t size, const char *tag);
  void *(*realloc)(void *context, void *ptr, size_t old_size, size_t new_size,
                   const char *tag);
  void (*free)(void *context, void *ptr, size_t size, const char *tag);
  void *context;
} Allocator;

#define ALLOC(a, size, tag) (a)->alloc((a)->context, size, tag)
#define REALLOC(a, ptr, old_size, new_size, tag)                               \
  (a)->realloc((a)->context, ptr, old_size, new_size, tag)
#define FREE(a, ptr, size, tag) (a)->free((a)->context, ptr, size, tag);

extern Allocator raw_allocator;

typedef struct LogSink {
  void (*log)(void *context, const char *fmt, va_list args);
  void *context;
} LogSink;

void console_log(void *context, const char *fmt, va_list args);
void file_log(void *context, const char *fmt, va_list args);

typedef struct AllocLog {
  void *ptr;
  size_t size;
  const char *tag;
  struct AllocLog *next;
} AllocLog;

typedef struct TracingContext {
  AllocLog *head;
  LogSink *sink;
  size_t allocated;
  size_t freed;
} TracingContext;

void *tracing_alloc(void *context, size_t size, const char *tag);
void *tracing_realloc(void *context, void *ptr, size_t old_size,
                      size_t new_size, const char *tag);
void tracing_free(void *context, void *ptr, size_t size, const char *tag);

void dump_memory_leaks(TracingContext *context);
