# Vault Specification

This document aims at outlining the protocol between Tor and its vault
implementation. The vault design is based on some of the same principles behind
the pluggable transports subsystem in Tor where Tor is responsible for spawning
a child process, that takes care of various cryptographic operations. Tor
communicates with the child process over the process' standard input, standard
output, and standard error file descriptors, which should make it easy for
people interested in building their own vault implementations to do so in the
language they prefer without having any knowledge of the internals of Tor
itself.

FIXME(ahf): Delete this before PR and create PR for torspec.git.

## TODO

- Add `LD_VAULT` instead of abusing `LD_PT`.
- Move Vault code from `src/lib` to `src/feature`.

.. vim: set spell spelllang=en tw=80 :
