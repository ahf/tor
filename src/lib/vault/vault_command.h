/* Copyright (c) 2019, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file vault_command.h
 * \brief Header file for vault_command.c.
 **/

#ifndef TOR_VAULT_COMMAND_H
#define TOR_VAULT_COMMAND_H

#include "lib/encoding/confline.h"

void vault_command_handle(const char *);

#ifdef VAULT_COMMAND_PRIVATE
STATIC void vault_command_handle_log(const config_line_t *);
#endif /* VAULT_COMMAND_PRIVATE */

#endif /* TOR_VAULT_COMMAND_H */

