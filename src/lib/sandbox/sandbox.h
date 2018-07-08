/* Copyright (c) 2001 Matej Pfajfar.
 * Copyright (c) 2001-2004, Roger Dingledine.
 * Copyright (c) 2004-2006, Roger Dingledine, Nick Mathewson.
 * Copyright (c) 2007-2018, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file sandbox.h
 * \brief Header file for sandbox.c.
 **/

#ifndef SANDBOX_H_
#define SANDBOX_H_

#include "orconfig.h"
#include "lib/cc/torint.h"
#include "lib/container/smartlist.h"

#ifndef SYS_SECCOMP

/**
 * Used by SIGSYS signal handler to check if the signal was issued due to a
 * seccomp2 filter violation.
 */
#define SYS_SECCOMP 1

#endif /* !defined(SYS_SECCOMP) */

#if defined(HAVE_SECCOMP_H) && defined(__linux__)
#define USE_LIBSECCOMP
#endif

/** Configuration for Sandbox. */
typedef struct sandbox_cfg_t {
  /** Sandbox config parameters (list of smp_param_t). */
  smartlist_t *parameters;
} sandbox_cfg_t;

/**
 * Linux definitions
 */
#ifdef USE_LIBSECCOMP

#include <sys/ucontext.h>
#include <seccomp.h>
#include <netdb.h>

/**
 *  Configuration parameter structure associated with the LIBSECCOMP2
 *  implementation.
 */
typedef struct smp_param {
  /** syscall associated with parameter. */
  int syscall;

  /** parameter value. */
  char *value;
  /** parameter value, second argument. */
  char *value2;

  /**  parameter flag (0 = not protected, 1 = protected). */
  int prot;
} smp_param_t;

#endif /* defined(USE_LIBSECCOMP) */

#ifdef USE_LIBSECCOMP
/** Returns a registered protected string used with the sandbox, given that
 * it matches the parameter.
 */
const char* sandbox_intern_string(const char *param);
#else /* !(defined(USE_LIBSECCOMP)) */
#define sandbox_intern_string(s) (s)
#endif /* defined(USE_LIBSECCOMP) */

/** Creates an empty sandbox configuration file.*/
sandbox_cfg_t* sandbox_cfg_new(void);

/**
 * Function used to add a open allowed filename to a supplied configuration.
 * The (char*) specifies the path to the allowed file; we take ownership
 * of the pointer.
 */
int sandbox_cfg_allow_open_filename(sandbox_cfg_t *cfg, char *file);

int sandbox_cfg_allow_chmod_filename(sandbox_cfg_t *cfg, char *file);
int sandbox_cfg_allow_chown_filename(sandbox_cfg_t *cfg, char *file);

/* DOCDOC */
int sandbox_cfg_allow_rename(sandbox_cfg_t *cfg, char *file1, char *file2);

/**
 * Function used to add a openat allowed filename to a supplied configuration.
 * The (char*) specifies the path to the allowed file; we steal the pointer to
 * that file.
 */
int sandbox_cfg_allow_openat_filename(sandbox_cfg_t *cfg, char *file);

/**
 * Function used to add a stat/stat64 allowed filename to a configuration.
 * The (char*) specifies the path to the allowed file; that pointer is stolen.
 */
int sandbox_cfg_allow_stat_filename(sandbox_cfg_t *cfg, char *file);

/** Function used to initialise a sandbox configuration.*/
int sandbox_init(sandbox_cfg_t *cfg);

/** Return true iff the sandbox is turned on. */
int sandbox_is_active(void);

#endif /* !defined(SANDBOX_H_) */
