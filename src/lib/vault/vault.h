/* Copyright (c) 2019, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file vault.h
 * \brief Header for vault.c
 **/

#ifndef TOR_VAULT_H
#define TOR_VAULT_H

#include <stdbool.h>

#include "orconfig.h"

void vault_init(void);
void vault_deinit(void);

bool vault_exec_configure(const char *path);

#ifdef VAULT_PRIVATE
#endif /* defined(VAULT_PRIVATE) */

#endif /* !defined(TOR_VAULT_H) */
