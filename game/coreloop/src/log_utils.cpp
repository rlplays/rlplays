#include "log.h"
#include "raylib.h"
#if RLPLAYS_TEST

void AddTestTraceLog(int level, const char* text, ...)
{
  if (TEST_TRACE_LOG_COUNTS_.find(level) == TEST_TRACE_LOG_COUNTS_.end())
  {
    TEST_TRACE_LOG_COUNTS_[level] = 0;
  }
  else
  {
    TEST_TRACE_LOG_COUNTS_[level] = TEST_TRACE_LOG_COUNTS_[level] + 1;
  }
#if DEBUG
  if (level >= LOG_WARNING)
  {
    // Break into the debugger if a severe error happens during tests.
    // Check console out for the actual log that was printed by TraceLog.
    TASSERT_BREAK();
  }
#endif
}

#endif
