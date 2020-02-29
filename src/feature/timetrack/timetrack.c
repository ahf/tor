/* Copyright (c) 2020, The Tor Project, Inc. */
/* See LICENSE for licensing information */

/**
 * \file timetrack.c
 * \brief XXX: Write a brief introduction to this module.
 **/

#define TIMETRACK_PRIVATE
#include "core/or/or.h"
#include "feature/control/control_events.h"
#include "feature/timetrack/timetrack.h"
#include "lib/container/order.h"
#include "lib/encoding/time_fmt.h"
#include "lib/log/log.h"

#include <stdlib.h>

#define N_OBSERVATIONS (3)

static struct {
  time_t wall_time;
  time_t observed_time;
} observations[N_OBSERVATIONS];

static uint64_t observation_counter = 0;

static time_t last_wall_time = 0;

void
timetrack_add_observation_impl(time_t wall_time, time_t observed_time)
{
  char buf[ISO_TIME_LEN + 1];

  /* Update our last wall timestamp. */
  last_wall_time = wall_time;

  format_local_iso_time(buf, observed_time);
  log_info(LD_TIMETRACK, "Learned of timestamp fro netinfo cell: %s", buf);

  /* Update our next slot. */
  uint64_t index = observation_counter++ % N_OBSERVATIONS;

  observations[index].wall_time = wall_time;
  observations[index].observed_time = observed_time;

  if (observation_counter >= N_OBSERVATIONS)
    timetrack_emit_timestamp(wall_time);
}

void
timetrack_emit_timestamp(time_t wall_time)
{
  /* We build an array of N_OBSERVATIONS containing the offset between
   * `wall_time` and `observed_time` and use that to compute the delta between
   * our current time and emit that.
   */
  int delta_time[N_OBSERVATIONS];

  for (int i = 0; i < N_OBSERVATIONS; ++i)
    delta_time[i] = observations[i].wall_time - observations[i].observed_time;

  qsort(delta_time, N_OBSERVATIONS, sizeof(int), timetrack_compare_int);

  int delta = median_int(delta_time, N_OBSERVATIONS);

  time_t timestamp = wall_time + delta;

  /* Emit value on the control port. */
  control_event_timetrack_timestamp(timestamp);

  /* Log for now ... */
  char buf[ISO_TIME_LEN + 1];
  format_local_iso_time(buf, timestamp);
  log_info(LD_TIMETRACK, "Emitting timestamp: %s", buf);
}

int
timetrack_compare_int(const void *a, const void *b)
{
  return (*(int *)a - *(int *)b);
}
