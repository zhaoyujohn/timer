//=-- lsan.cpp ------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of LeakSanitizer.
// Standalone LSan RTL.
//
//===----------------------------------------------------------------------===//

#include "lsan.h"

#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_flag_parser.h"
#include "lsan_allocator.h"
#include "lsan_common.h"
#include "lsan_thread.h"

bool lsan_inited;
bool lsan_init_is_running;

extern "C" {
__thread bool lsan_is_coroutine = false;
__thread void* coroutine_stack_top;
__thread void* coroutine_stack_bottom;

void lsan_set_coroutine(bool is_coroutine)
{
  lsan_is_coroutine = is_coroutine;
}

bool lsan_is_current_context_coroutine()
{
    return lsan_is_coroutine;
}

void lsan_set_coroutine_stack_top_and_end(void* stack_top, void* stack_bottom)
{
    coroutine_stack_top = stack_top;
    coroutine_stack_bottom = stack_bottom;
}

void* lsan_get_coroutine_stack_top()
{
    return coroutine_stack_top;
}

void* lsan_get_coroutine_stack_bottom()
{
    return coroutine_stack_bottom;
}

}


namespace __lsan {

///// Interface to the common LSan module. /////
bool WordIsPoisoned(uptr addr) {
  return false;
}

}  // namespace __lsan

void __sanitizer::BufferedStackTrace::UnwindImpl(
    uptr pc, uptr bp, void *context, bool request_fast, u32 max_depth) {
  using namespace __lsan;
  uptr stack_top = 0, stack_bottom = 0;
  if (ThreadContext *t = CurrentThreadContext()) {
    if (lsan_is_current_context_coroutine()) {
        stack_top = (uptr)lsan_get_coroutine_stack_top();
        stack_bottom = (uptr)lsan_get_coroutine_stack_bottom();
    } else {
        stack_top = t->stack_end();
        stack_bottom = t->stack_begin();
    }
  }
  if (SANITIZER_MIPS && !IsValidFrame(bp, stack_top, stack_bottom))
    return;
  bool fast = StackTrace::WillUseFastUnwind(request_fast);
  Unwind(max_depth, pc, bp, context, stack_top, stack_bottom, fast);
}

using namespace __lsan;

static void InitializeFlags() {
  // Set all the default values.
  SetCommonFlagsDefaults();
  {
    CommonFlags cf;
    cf.CopyFrom(*common_flags());
    cf.external_symbolizer_path = GetEnv("LSAN_SYMBOLIZER_PATH");
    cf.malloc_context_size = 30;
    cf.intercept_tls_get_addr = true;
    cf.detect_leaks = true;
    cf.exitcode = 23;
    OverrideCommonFlags(cf);
  }

  Flags *f = flags();
  f->SetDefaults();

  FlagParser parser;
  RegisterLsanFlags(&parser, f);
  RegisterCommonFlags(&parser);

  // Override from user-specified string.
  const char *lsan_default_options = __lsan_default_options();
  parser.ParseString(lsan_default_options);
  parser.ParseStringFromEnv("LSAN_OPTIONS");

  InitializeCommonFlags();

  if (Verbosity()) ReportUnrecognizedFlags();

  if (common_flags()->help) parser.PrintFlagDescriptions();

  __sanitizer_set_report_path(common_flags()->log_path);
}

extern "C" void __lsan_init() {
  CHECK(!lsan_init_is_running);
  if (lsan_inited)
    return;
  lsan_init_is_running = true;
  SanitizerToolName = "LeakSanitizer";
  CacheBinaryName();
  AvoidCVE_2016_2143();
  InitializeFlags();
  InitCommonLsan();
  InitializeAllocator();
  ReplaceSystemMalloc();
  InitTlsSize();
  InitializeInterceptors();
  InitializeThreadRegistry();
  InstallDeadlySignalHandlers(LsanOnDeadlySignal);
  InitializeMainThread();

  if (common_flags()->detect_leaks && common_flags()->leak_check_at_exit)
    Atexit(DoLeakCheck);

  InitializeCoverage(common_flags()->coverage, common_flags()->coverage_dir);

  lsan_inited = true;
  lsan_init_is_running = false;
}

extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __sanitizer_print_stack_trace() {
  GET_STACK_TRACE_FATAL;
  stack.Print();
}
