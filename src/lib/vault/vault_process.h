/* Copyright (c) 2019, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file vault_process.h
 * \brief Header file for vault_process.c.
 **/

#ifndef TOR_VAULT_PROCESS_H
#define TOR_VAULT_PROCESS_H

#include "lib/encoding/confline.h"
#include "lib/process/process.h"

void vault_process_init(void);
void vault_process_deinit(void);

bool vault_process_configure(const char *);

#ifdef VAULT_PROCESS_PRIVATE
STATIC void vault_process_notify_version(void);
STATIC void vault_process_write(const config_line_t *);
STATIC void vault_process_stdout_callback(process_t *, const char *, size_t);
STATIC void vault_process_stderr_callback(process_t *, const char *, size_t);
STATIC bool vault_process_exit_callback(process_t *, process_exit_code_t);
#endif /* VAULT_PROCESS_PRIVATE */

#endif /* TOR_VAULT_PROCESS_H */

