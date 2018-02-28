Stats Reporter
==============

FIXME(ahf): Remove this document.

## TODO

- StatsReporter connections should reconnect when they disconnect from their
  service.

- We should support UDP and UNIX sockets for StatsReporter services.

- Add a `stats_entry_t` instance which supports wrapping a function `size_t
  fun(void)` to support a pull-model for counters until things are able to use
  the `stats_entry` API.

- Fix layering violations between `src/common/stats_reporter.c` and its
  dependencies on things in `src/or/`.
