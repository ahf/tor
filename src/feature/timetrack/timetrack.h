/* Copyright (c) 2020, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file timetrack.h
 * \brief Header file for timetrack.c.
 **/

#ifndef TOR_TIMETRACK_H
#define TOR_TIMETRACK_H

#include "lib/wallclock/approx_time.h"

#define timetrack_add_observation(observed_time) \
  timetrack_add_observation_impl(approx_time(), (observed_time))

void timetrack_add_observation_impl(time_t wall_time, time_t observed_time);

#ifdef TIMETRACK_PRIVATE
void timetrack_emit_timestamp(time_t wall_time);
int timetrack_compare_int(const void *a, const void *b);
#endif /* TIMETRACK_PRIVATE */

#endif /* TOR_TIMETRACK_H */

