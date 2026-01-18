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

#include "allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *raw_alloc(void *context, size_t size, const char *tag) {
  (void)context;
  (void)tag;
  return malloc(size);
}

void *raw_realloc(void *context, void *ptr, size_t old_size, size_t new_size,
                  const char *tag) {
  (void)context;
  (void)old_size;
  (void)tag;
  return realloc(ptr, new_size);
}

void raw_free(void *context, void *ptr, size_t size, const char *tag) {
  (void)context;
  (void)size;
  (void)tag;
  free(ptr);
}

Allocator raw_allocator = {raw_alloc, raw_realloc, raw_free, NULL};

void sink_println(LogSink *sink, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  sink->log(sink->context, fmt, args);
  va_end(args);
}

void console_log(void *context, const char *fmt, va_list args) {
  (void)context;
  vprintf(fmt, args);
  printf("\n");
}

void file_log(void *context, const char *fmt, va_list args) {
  FILE *file = (FILE *)context;
  if (file) {
    vfprintf(file, fmt, args);
    fprintf(file, "\n");
    fflush(file);
  }
}

void *tracing_alloc(void *context, size_t size, const char *tag) {
  TracingContext *tc = (TracingContext *)context;
  void *ptr = malloc(size);

  if (ptr) {
    AllocLog *log = malloc(sizeof(AllocLog));
    if (log) {
      log->ptr = ptr;
      log->size = size;
      log->tag = tag;
      log->next = tc->head;
      tc->head = log;
    }

    sink_println(tc->sink, "Allocated %zu bytes for %s at %p", size, tag, ptr);
    tc->allocated += size;
  } else {
    sink_println(tc->sink, "Failed to allocate %zu bytes for %s", size, tag);
  }

  return ptr;
}

void *tracing_realloc(void *context, void *ptr, size_t old_size,
                      size_t new_size, const char *tag) {
  TracingContext *tc = (TracingContext *)context;

  if (ptr == NULL)
    return tracing_alloc(tc, new_size, tag);

  if (new_size == 0) {
    tracing_free(tc, ptr, old_size, tag);
    return NULL;
  }

  void *new_ptr = tracing_alloc(tc, new_size, tag);
  if (new_ptr == NULL)
    return NULL;

  size_t copy_size = (old_size < new_size) ? old_size : new_size;
  memcpy(new_ptr, ptr, copy_size);

  tracing_free(tc, ptr, old_size, tag);

  return NULL;
}

void tracing_free(void *context, void *ptr, size_t size, const char *tag) {
  TracingContext *tc = (TracingContext *)context;

  free(ptr);
  tc->freed += size;

  AllocLog *curr = tc->head;
  AllocLog *prev = NULL;
  while (curr) {
    if (curr->ptr == ptr) {
      if (prev) {
        prev->next = curr->next;
      } else {
        tc->head = curr->next;
      }
      free(curr);
      break;
    }
    prev = curr;
    curr = curr->next;
  }

  sink_println(tc->sink, "Freed %zu bytes for %s at %p", size, tag, ptr);
}

void dump_memory_leaks(TracingContext *context) {
  sink_println(context->sink, "--- MEMORY LEAK REPORT ---");

  if (context->head == NULL) {
    sink_println(context->sink,
                 "No memory leaks detected! All memory is accounted for :)");
  } else {
    AllocLog *curr = context->head;
    while (curr) {
      sink_println(context->sink, "Leaked %zu bytes for %s at %p", curr->size,
                   curr->tag, curr->ptr);
      AllocLog *tmp = curr->next;
      free(curr);
      curr = tmp;
    }
  }

  size_t leaked = context->allocated - context->freed;
  sink_println(context->sink, "Total allocated: %zu bytes", context->allocated);
  sink_println(context->sink, "Total freed:     %zu bytes", context->freed);
  sink_println(context->sink, "Memory leaked:   %zu bytes", leaked);
  sink_println(context->sink, "--------------------------");
}
