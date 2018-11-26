/* Copyright (c) 2018, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file test_process_slow.c
 * \brief Slow test cases for the Process API.
 */

#include "orconfig.h"
#include "core/or/or.h"
#include "core/mainloop/mainloop.h"
#include "lib/evloop/compat_libevent.h"
#include "lib/process/process.h"
#include "lib/process/waitpid.h"
#include "test/test.h"

#ifndef BUILDDIR
#define BUILDDIR "."
#endif

#ifdef _WIN32
#define TEST_PROCESS "test-process.exe"
#else
#define TEST_PROCESS BUILDDIR "/src/test/test-process"
#endif /* defined(_WIN32) */

/** Timer that ticks once a second and stop the event loop after 5 ticks. */
static periodic_timer_t *main_loop_timeout_timer;

struct process_data_t {
  smartlist_t *stdout_data;
  smartlist_t *stderr_data;
  smartlist_t *stdin_data;
  process_exit_code_t exit_code;
};

typedef struct process_data_t process_data_t;

static process_data_t *
process_data_new(void)
{
  process_data_t *process_data = tor_malloc_zero(sizeof(process_data_t));
  process_data->stdout_data = smartlist_new();
  process_data->stderr_data = smartlist_new();
  process_data->stdin_data = smartlist_new();
  return process_data;
}

static void
process_data_free(process_data_t *process_data)
{
  if (process_data == NULL)
    return;

  SMARTLIST_FOREACH(process_data->stdout_data, char *, x, tor_free(x));
  SMARTLIST_FOREACH(process_data->stderr_data, char *, x, tor_free(x));
  SMARTLIST_FOREACH(process_data->stdin_data, char *, x, tor_free(x));

  smartlist_free(process_data->stdout_data);
  smartlist_free(process_data->stderr_data);
  smartlist_free(process_data->stdin_data);
  tor_free(process_data);
}

static void
process_stdout_callback(process_t *process, char *data, size_t size)
{
  tor_assert(process);
  tor_assert(data);

  (void)size;

  process_data_t *process_data = process_get_data(process);
  smartlist_add(process_data->stdout_data, tor_strdup(data));
}

static void
process_stderr_callback(process_t *process, char *data, size_t size)
{
  tor_assert(process);
  tor_assert(data);

  (void)size;

  process_data_t *process_data = process_get_data(process);
  smartlist_add(process_data->stderr_data, tor_strdup(data));
}

static void
process_exit_callback(process_t *process, process_exit_code_t exit_code)
{
  tor_assert(process);

  process_data_t *process_data = process_get_data(process);
  process_data->exit_code = exit_code;

  /* Our process died. Let's check the values it returned. */
  tor_shutdown_event_loop_and_exit(0);
}

static void
main_loop_timeout_cb(periodic_timer_t *timer, void *data)
{
  /* Sanity check. */
  tt_ptr_op(timer, OP_EQ, main_loop_timeout_timer);
  tt_ptr_op(data, OP_EQ, NULL);

  /* Have we been called 5 times we exit. */
  static int max_calls = 0;

  tt_int_op(max_calls, OP_LT, 5);
  ++max_calls;

#ifndef _WIN32
  /* Call waitpid callbacks. */
  notify_pending_waitpid_callbacks();
#endif

  return;
 done:
  /* Exit with an error. */
  max_calls = 0;
  tor_shutdown_event_loop_and_exit(-1);
}

static void
run_main_loop(void)
{
  int ret;

  /* Wake up after 5 seconds. */
  static const struct timeval interval = {1, 0};

  main_loop_timeout_timer = periodic_timer_new(tor_libevent_get_base(),
                                               &interval,
                                               main_loop_timeout_cb,
                                               NULL);

  /* Run our main loop. */
  ret = do_main_loop();

  /* Clean up our main loop timeout timer. */
  tt_int_op(ret, OP_EQ, 0);

 done:
  periodic_timer_free(main_loop_timeout_timer);
}

static void
test_callbacks(void *arg)
{
  (void)arg;

  /* Initialize Process subsystem. */
  process_init();

  /* Process callback data. */
  process_data_t *process_data = process_data_new();

  /* Setup our process. */
  process_t *process = process_new();
  process_set_name(process, TEST_PROCESS);
  process_set_data(process, process_data);
  process_set_stdout_read_callback(process, process_stdout_callback);
  process_set_stderr_read_callback(process, process_stderr_callback);
  process_set_exit_callback(process, process_exit_callback);

  /* Set environment variable. */
  process_set_environment(process, "TOR_TEST_ENV", "Hello, from Tor!");

  /* Add some arguments. */
  process_append_argument(process, "This is the first one");
  process_append_argument(process, "Second one");
  process_append_argument(process, "Third: Foo bar baz");

  /* Run our process. */
  process_status_t status;

  status = process_exec(process);
  tt_int_op(status, OP_EQ, PROCESS_STATUS_RUNNING);

  /* Write some lines to stdin. */
  process_printf(process, "Hi process!\r\n");
  process_printf(process, "Can you read more than one line?\n");
  process_printf(process, "Can you read partial ...");
  process_printf(process, " lines?\r\n");

  /* Start our main loop. */
  run_main_loop();

  /* Check if our process is still running? */
  status = process_get_status(process);
  tt_int_op(status, OP_EQ, PROCESS_STATUS_NOT_RUNNING);

  /* We returned. Let's see what our event loop said. */
  tt_int_op(smartlist_len(process_data->stdout_data), OP_EQ, 12);
  tt_int_op(smartlist_len(process_data->stderr_data), OP_EQ, 3);
  tt_int_op(process_data->exit_code, OP_EQ, 0);

  /* Check stdout output. */
  tt_str_op(smartlist_get(process_data->stdout_data, 0), OP_EQ,
            "argv[0] = '" TEST_PROCESS "'");
  tt_str_op(smartlist_get(process_data->stdout_data, 1), OP_EQ,
            "argv[1] = 'This is the first one'");
  tt_str_op(smartlist_get(process_data->stdout_data, 2), OP_EQ,
            "argv[2] = 'Second one'");
  tt_str_op(smartlist_get(process_data->stdout_data, 3), OP_EQ,
            "argv[3] = 'Third: Foo bar baz'");
  tt_str_op(smartlist_get(process_data->stdout_data, 4), OP_EQ,
            "Environment variable TOR_TEST_ENV = 'Hello, from Tor!'");
  tt_str_op(smartlist_get(process_data->stdout_data, 5), OP_EQ,
            "Output on stdout");
  tt_str_op(smartlist_get(process_data->stdout_data, 6), OP_EQ,
            "This is a new line");
  tt_str_op(smartlist_get(process_data->stdout_data, 7), OP_EQ,
            "Partial line on stdout ...end of partial line on stdout");
  tt_str_op(smartlist_get(process_data->stdout_data, 8), OP_EQ,
            "Read line from stdin: 'Hi process!'");
  tt_str_op(smartlist_get(process_data->stdout_data, 9), OP_EQ,
            "Read line from stdin: 'Can you read more than one line?'");
  tt_str_op(smartlist_get(process_data->stdout_data, 10), OP_EQ,
            "Read line from stdin: 'Can you read partial ... lines?'");
  tt_str_op(smartlist_get(process_data->stdout_data, 11), OP_EQ,
            "We are done for here, thank you!");

  /* Check stderr output. */
  tt_str_op(smartlist_get(process_data->stderr_data, 0), OP_EQ,
            "Output on stderr");
  tt_str_op(smartlist_get(process_data->stderr_data, 1), OP_EQ,
            "This is a new line");
  tt_str_op(smartlist_get(process_data->stderr_data, 2), OP_EQ,
            "Partial line on stderr ...end of partial line on stderr");

 done:
  process_data_free(process_data);
  process_free(process);
  process_free_all();
}

static void
test_callbacks_terminate(void *arg)
{
  (void)arg;

  /* Initialize Process subsystem. */
  process_init();

  /* Process callback data. */
  process_data_t *process_data = process_data_new();

  /* Setup our process. */
  process_t *process = process_new();
  process_set_name(process, TEST_PROCESS);
  process_set_data(process, process_data);
  process_set_exit_callback(process, process_exit_callback);

  /* Run our process. */
  process_status_t status;

  status = process_exec(process);
  tt_int_op(status, OP_EQ, PROCESS_STATUS_RUNNING);

  /* Zap our process. */
  process_terminate(process);

  /* Start our main loop. */
  run_main_loop();

  /* Check if our process is still running? */
  status = process_get_status(process);
  tt_int_op(status, OP_EQ, PROCESS_STATUS_NOT_RUNNING);

  /* Check return code. */
#ifndef _WIN32
  tt_int_op(process_data->exit_code, OP_EQ, SIGTERM);
#else
  tt_int_op(process_data->exit_code, OP_EQ, 0);
#endif

 done:
  process_data_free(process_data);
  process_free(process);
  process_free_all();
}

struct testcase_t slow_process_tests[] = {
  { "callbacks", test_callbacks, 0, NULL, NULL },
  { "callbacks_terminate", test_callbacks_terminate, 0, NULL, NULL },
  END_OF_TESTCASES
};
