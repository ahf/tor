/* Copyright (c) 2019, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file vault_process.c
 * \brief Module to handle Vault processes.
 **/

#define VAULT_PROCESS_PRIVATE
#include "lib/encoding/kvline.h"
#include "lib/log/log.h"
#include "lib/log/util_bug.h"
#include "lib/process/process.h"
#include "lib/vault/vault_command.h"
#include "lib/vault/vault_process.h"
#include "lib/version/torversion.h"

// FIXME(ahf): Lazy ...
#define LD_VAULT LD_PT

/** The Vault process. */
static process_t *vault_process = NULL;

/** Initialize the Vault process. */
void
vault_process_init(void)
{
}

/** Deinitialize the Vault process. */
void
vault_process_deinit(void)
{
  /* Terminate vault process. */
  if (vault_process != NULL) {
    process_terminate(vault_process);
    vault_process = NULL;
  }
}

/** Configure and start our Vault process. */
bool
vault_process_configure(const char *path)
{
  tor_assert(path);

  /* If we already have a running Vault process we terminate it. */
  if (vault_process != NULL) {
    /* The exit handler for the currently running Vault will free up its own
     * process_t memory. */
    process_terminate(vault_process);
    vault_process = NULL;
  }

  /* Configure our new Vault. */
  vault_process = process_new(path);
  process_set_stdout_read_callback(vault_process,
                                   vault_process_stdout_callback);
  process_set_stderr_read_callback(vault_process,
                                   vault_process_stderr_callback);
  process_set_exit_callback(vault_process, vault_process_exit_callback);
  process_set_protocol(vault_process, PROCESS_PROTOCOL_LINE);
  process_append_argument(vault_process, path);

  /* Start our new Vault. */
  if (process_exec(vault_process) != PROCESS_STATUS_RUNNING) {
    log_warn(LD_VAULT, "Unable to start Vault process '%s'", path);
    return false;
  }

  /* Tell the Vault process about our version of Tor. */
  vault_process_notify_version();

  return true;
}

/** Notify our Vault process about the version of Tor. */
STATIC void
vault_process_notify_version(void)
{
  config_line_t *values = NULL;

  config_line_append(&values, "CMD", "VERSION");
  config_line_append(&values, "VERSION", get_version());

  vault_process_write(values);

  config_free_lines(values);
}

/** Write the given K/V pairs found in <b>values</b> to our Vault process. */
STATIC void
vault_process_write(const config_line_t *values)
{
  tor_assert(values);

  if (! vault_process)
    return;

  char *data = kvline_encode(values, KV_QUOTED);
  log_debug(LD_VAULT, "Sending Vault command: '%s'", data);
  process_printf(vault_process, "%s\n", data);
  tor_free(data);
}

/** Callback function that is called when our Vault process have data on
 *  stdout. */
STATIC void
vault_process_stdout_callback(process_t *process,
                              const char *line,
                              size_t size)
{
  tor_assert(process);
  tor_assert(line);

  (void)size;

  log_debug(LD_VAULT, "Vault process wrote '%s'", line);

  /* Handle the new command. */
  vault_command_handle(line);
}

/** Callback function that is called when our Vault process have data on
 *  stderr. */
STATIC void
vault_process_stderr_callback(process_t *process,
                              const char *line,
                              size_t size)
{
  tor_assert(process);
  tor_assert(line);

  (void)size;

  log_warn(LD_VAULT, "Vault reported '%s' on its standard error", line);
}

/** Callback function that is called when our Vault process terminated. */
STATIC bool
vault_process_exit_callback(process_t *process,
                            process_exit_code_t exit_code)
{
  tor_assert(process);

  log_warn(LD_VAULT, "Vault process terminated with status code %" PRIu64,
           exit_code);

  /* Returning true here means that the process subsystem will call
   * process_free() on our process_t. */
  return true;
}
