/* Copyright (c) 2003-2004, Roger Dingledine
 * Copyright (c) 2004-2006, Roger Dingledine, Nick Mathewson.
 * Copyright (c) 2007-2015, The Tor Project, Inc. */
/* See LICENSE for licensing information */

#include "compat.h"
#include <windows.h>
#include <process.h>
#include "util.h"
#include "container.h"
#include "torlog.h"

/** Minimalist interface to run a void function in the background.  On
 * Unix calls fork, on win32 calls beginthread.  Returns -1 on failure.
 * func should not return, but rather should call spawn_exit.
 *
 * NOTE: if <b>data</b> is used, it should not be allocated on the stack,
 * since in a multithreaded environment, there is no way to be sure that
 * the caller's stack will still be around when the called function is
 * running.
 */
int
spawn_func(void (*func)(void *), void *data)
{
  int rv;
  rv = (int)_beginthread(func, 0, data);
  if (rv == (int)-1)
    return -1;
  return 0;
}


/** End the current thread/process.
 */
void
spawn_exit(void)
{
  _endthread();
  //we should never get here. my compiler thinks that _endthread returns, this
  //is an attempt to fool it.
  tor_assert(0);
  _exit(0);
}


void
tor_mutex_init(tor_mutex_t *m)
{
  InitializeCriticalSection(&m->mutex);
}
void
tor_mutex_init_for_cond(tor_mutex_t *m)
{
  InitializeCriticalSection(&m->mutex);
}

void
tor_mutex_uninit(tor_mutex_t *m)
{
  DeleteCriticalSection(&m->mutex);
}
void
tor_mutex_acquire(tor_mutex_t *m)
{
  tor_assert(m);
  EnterCriticalSection(&m->mutex);
}
void
tor_mutex_release(tor_mutex_t *m)
{
  LeaveCriticalSection(&m->mutex);
}
unsigned long
tor_get_thread_id(void)
{
  return (unsigned long)GetCurrentThreadId();
}

int
tor_cond_init(tor_cond_t *cond)
{
  memset(cond, 0, sizeof(tor_cond_t));
  if (InitializeCriticalSectionAndSpinCount(&cond->lock, SPIN_COUNT)==0) {
    return -1;
  }
  if ((cond->event = CreateEvent(NULL,TRUE,FALSE,NULL)) == NULL) {
    DeleteCriticalSection(&cond->lock);
    return -1;
  }
  cond->n_waiting = cond->n_to_wake = cond->generation = 0;
  return 0;
}
void
tor_cond_uninit(tor_cond_t *cond)
{
  DeleteCriticalSection(&cond->lock);
  CloseHandle(cond->event);
}

static void
tor_cond_signal_impl(tor_cond_t *cond, int broadcast)
{
  EnterCriticalSection(&cond->lock);
  if (broadcast)
    cond->n_to_wake = cond->n_waiting;
  else
    ++cond->n_to_wake;
  cond->generation++;
  SetEvent(cond->event);
  LeaveCriticalSection(&cond->lock);
  return 0;
}
void
tor_cond_signal_one(tor_cond_t *cond)
{
  tor_cond_signal_impl(cond, 0);
}
void
tor_cond_signal_all(tor_cond_t *cond)
{
  tor_cond_signal_impl(cond, 1);
}

int
tor_cond_wait(tor_cond_t *cond, tor_mutex_t *lock, const struct timeval *tv)
{
  CRITICAL_SECTION *lock = &lock->mutex;
  int generation_at_start;
  int waiting = 1;
  int result = -1;
  DWORD ms = INFINITE, ms_orig = INFINITE, startTime, endTime;
  if (tv)
    ms_orig = ms = evutil_tv_to_msec_(tv);

  EnterCriticalSection(&cond->lock);
  ++cond->n_waiting;
  generation_at_start = cond->generation;
  LeaveCriticalSection(&cond->lock);

  LeaveCriticalSection(lock);

  startTime = GetTickCount();
  do {
    DWORD res;
    res = WaitForSingleObject(cond->event, ms);
    EnterCriticalSection(&cond->lock);
    if (cond->n_to_wake &&
        cond->generation != generation_at_start) {
      --cond->n_to_wake;
      --cond->n_waiting;
      result = 0;
      waiting = 0;
      goto out;
    } else if (res != WAIT_OBJECT_0) {
      result = (res==WAIT_TIMEOUT) ? 1 : -1;
      --cond->n_waiting;
      waiting = 0;
      goto out;
    } else if (ms != INFINITE) {
      endTime = GetTickCount();
      if (startTime + ms_orig <= endTime) {
        result = 1; /* Timeout */
        --cond->n_waiting;
        waiting = 0;
        goto out;
      } else {
        ms = startTime + ms_orig - endTime;
      }
    }
    /* If we make it here, we are still waiting. */
    if (cond->n_to_wake == 0) {
      /* There is nobody else who should wake up; reset
       * the event. */
      ResetEvent(cond->event);
    }
  out:
    LeaveCriticalSection(&cond->lock);
  } while (waiting);

  EnterCriticalSection(lock);

  EnterCriticalSection(&cond->lock);
  if (!cond->n_waiting)
    ResetEvent(cond->event);
  LeaveCriticalSection(&cond->lock);

  return result;
}

void
tor_threads_init(void)
{
  set_main_thread();
}
