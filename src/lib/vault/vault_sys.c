/* Copyright (c) 2019, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file vault_sys.c
 * \brief Subsystem object for vault setup.
 **/

#include "orconfig.h"
#include "lib/subsys/subsys.h"
#include "lib/vault/vault_sys.h"
#include "lib/vault/vault.h"

static int
subsys_vault_initialize(void)
{
  vault_init();
  return 0;
}

static void
subsys_vault_shutdown(void)
{
  vault_deinit();
}

const subsys_fns_t sys_vault = {
  .name = "vault",
  .level = -34,
  .supported = true,
  .initialize = subsys_vault_initialize,
  .shutdown = subsys_vault_shutdown
};
