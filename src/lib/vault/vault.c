/* Copyright (c) 2019, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file vault.c
 * \brief Module for handling Vault processes.
 **/

#define VAULT_PRIVATE
#include "lib/log/log.h"
#include "lib/log/util_bug.h"
#include "lib/vault/vault.h"
#include "lib/vault/vault_process.h"

/** Initialize the Vault module. */
void
vault_init(void)
{
  vault_process_init();
}

/** Configure and start an executable Vault. */
bool
vault_exec_configure(const char *path)
{
  tor_assert(path);

  return vault_process_configure(path);
}

/** Deinitialize the Vault module. */
void
vault_deinit(void)
{
  vault_process_deinit();
}
