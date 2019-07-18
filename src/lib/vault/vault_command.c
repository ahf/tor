/* Copyright (c) 2019, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file vault_command.c
 * \brief Module for handling Vault commands.
 **/

#define VAULT_COMMAND_PRIVATE
#include "lib/encoding/kvline.h"
#include "lib/log/log.h"
#include "lib/log/util_bug.h"
#include "lib/vault/vault.h"
#include "lib/vault/vault_command.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

// FIXME(ahf): Lazy ...
#define LD_VAULT LD_PT

/** Handle a given Vault command found in <b>line</b>. */
void
vault_command_handle(const char *line)
{
  tor_assert(line);

  config_line_t *values = kvline_parse(line, KV_QUOTED);

  if (! values) {
    log_warn(LD_VAULT, "Vault wrote an invalid message: '%s'", line);
    goto done;
  }

  const config_line_t *command = config_line_find(values, "CMD");

  if (! command) {
    log_warn(LD_VAULT, "Vault process wrote a line without a 'CMD' key");
    goto done;
  }

  if (! strcmp(command->value, "LOG")) {
    vault_command_handle_log(values);
    goto done;
  }

 done:
  config_free_lines(values);
}

/** Handle the LOG command from the Vault. */
STATIC void
vault_command_handle_log(const config_line_t *values)
{
  tor_assert(values);

  const config_line_t *severity = config_line_find(values, "SEVERITY");
  const config_line_t *message = config_line_find(values, "MESSAGE");

  if (! severity) {
    log_warn(LD_VAULT, "Vault LOG message without 'SEVERITY' key");
    goto err;
  }

  if (! message) {
    log_warn(LD_VAULT, "Vault LOG message without 'MESSAGE' key");
    goto err;
  }

  int log_level = parse_log_level(severity->value);

  if (log_level == -1) {
    log_warn(LD_VAULT, "Vault LOG message with invalid SEVERITY key: %s",
             severity->value);
    goto err;
  }

  tor_log(log_level, LD_VAULT, "Vault: %s", message->value);

 err:
  return;
}
