// Perf : (for my env on Ubuntu 24.04 13th Gen Intel(R) Core(TM) i9-13900HK 20 cores / nVidia 1080Ti)
// Single-threaded (before):            ~34K sps (74% CPU utilization GPU 12%)
// 4-threads  (after, using this code): ~78K sps (200% CPU utiliation GPU 35%)
// 8-threads  (after, using this code): ~98K sps (285% CPU utiliation GPU 45%)
// 10-threads (after, using this code): ~110K sps (300% CPU utiliation GPU 45%)
// Beyond 10 threads, diminishing returns (For my system - which is a cheap beelink mini pc with a 1080Ti)
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#ifndef PUFFERLIB_NUM_THREADS
#define PUFFERLIB_NUM_THREADS (8)
#endif
using namespace std;

#ifdef _WIN32
// The shortest/simplest cross-platform friendly pthreads for Windows.
// The native threads etc is a nightmare to use on Windows with many gotchas.
// Rather we use the pthreads as is, and most of the C++ version is a 1:1 match.
typedef std::condition_variable pthread_cond_t;
typedef std::thread pthread_t;
typedef std::mutex pthread_mutex_t;

static void pthread_mutex_init(pthread_mutex_t* mtx, void* p) {}
static void pthread_mutex_lock(pthread_mutex_t* mtx) { mtx->lock(); }
static void pthread_mutex_unlock(pthread_mutex_t* mtx) { mtx->unlock(); }
static void pthread_mutex_destroy(pthread_mutex_t* mtx) {}

static void pthread_cond_wait(pthread_cond_t* cnd, pthread_mutex_t* mtx)
{
  std::unique_lock<std::mutex> temp_lock(*mtx, std::adopt_lock);
  cnd->wait(temp_lock);
  temp_lock.release();
}

static int pthread_cond_init(pthread_cond_t* cnd, void* p) { return 0; }
static void pthread_cond_destroy(pthread_cond_t* cnd) {}

static void pthread_cond_broadcast(pthread_cond_t* cnd) { cnd->notify_all(); }
static void pthread_cond_signal(pthread_cond_t* cnd) { cnd->notify_one(); }

static void pthread_join(pthread_t& thread, void* p) { thread.join(); }

static int pthread_create(pthread_t* thread, void* p, void* (*start_routine)(void*), void* arg)
{
  *thread = std::thread(start_routine, arg);
  return 0;
}
#endif

#include "env_glue.h"
