#pragma once


#define SUPPORT_TRACELOG 1

#include "utils.h"

#if RLPLAYS_TEST
#include <map>
static std::map<int, int> TEST_TRACE_LOG_COUNTS_;

// Defined in test_main.cpp for tests to catch errors.
void AddTestTraceLog(int level, const char *text, ...);

#define TLOG(Level, ...) \
  TRACELOG(Level, __VA_ARGS__); \
  AddTestTraceLog(Level, __VA_ARGS__)

#else
#define TLOG(Level, ...) TRACELOG(Level, __VA_ARGS__)
#endif


inline static void TASSERT_BREAK()
{
#if defined(_MSC_VER)
  // assert (abort) does not break into the debugger in VS 2022 ! It's insane, so we have to use this weird contraption that's cross platform.
  __debugbreak();
#elif defined(__clang__) || defined(__GNUC__)
  __builtin_trap();
#else
  /* Fallback method */
  *((volatile int*)0) = 0;  /* This will cause a segmentation fault */
#endif
}

// assert (abort) does not break into the debugger in VS 2022 ! It's insane, so we have to use this weird contraption that's cross platform.
#define TASSERT(condition, ...) \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
      TLOG(LOG_INFO, "Assertion failed: %s", #condition);                                                              \
      TASSERT_BREAK();                                                                                                 \
    }                                                                                                                  \
  }                                                                                                                    \
  while (0)

#define TINFO LOG_INFO
#define TDEBUG LOG_DEBUG
#define TWARNING LOG_WARNING
#define TERROR LOG_ERROR
