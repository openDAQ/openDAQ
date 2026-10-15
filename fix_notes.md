# Pin the LT Modern module to the streaming client fixes

Plan: `openDAQ-docs/Planning/LT-streaming-client-fixes.md`; change log: `LT-streaming-client-fixes-changes.md`.

## Bug

The LT streaming client (`WsStreaming` in LTStreamingModulesModern) connects in its constructor and publishes signals before the pseudo-device has connected its handlers. Over HTTP command interfaces, ws-streaming sends every subscribe and unsubscribe at once and ignores the answers; a digiBOX-WT drops some and reorders others. One flag (`isSubscribed`) stands for both openDAQ's request and the wire state. Symptoms: signals missing from the pseudo-device under CPU load (openDAQ #1318, macOS lanes), closing blocked for up to 4.7 s, every signal streamed for 1.5-3 s after it appears, and subscriptions that end without an unsubscribe.

## Fix

The fixes live in LT Modern (`draft-fix/streaming-client-fixes` @ 2709e9f, on LT main @ e1bd68b) and ws-streaming (`draft-fix/http-command-requests` @ d9635cc, on ws-streaming main @ 20b8597; 3.2.1, not tagged). LT Modern connects in a separate `connect()` after the handlers. ws-streaming sends HTTP command requests one at a time, in order, with one retry. Acks reach openDAQ only for its latest request. A signal publishes as soon as its descriptor is known and is linked to its domain signal later. The sweep timer, hold and replay workarounds from LT #16, #19 and #21 are gone. This branch only pins LT Modern to those fixes, for CI:

```diff
             GIT_REPOSITORY https://github.com/openDAQ/LTStreamingModulesModern.git
-            GIT_TAG        e1bd68b5041999fbf587128121f0f5f573760485
+            GIT_TAG        2709e9fda5eb5ab58de0e108df41ede19cbbc111
```

LT Modern pins ws-streaming `d9635cc` and keeps main's `GIT_SHALLOW OFF` (#31). Both SHAs are on GitHub on their `draft-fix/` branches. The earlier, unrebased revisions stay on the remotes: LT `fix/streaming-client-fixes` @ 9a0630b, ws-streaming `fix/http-command-requests` @ 1fded35.

## Tests

- openDAQ: `StreamingTestForModernLt` in `tests/integration/test_opendaq_device_modules/test_streaming.cpp` on the macOS lanes at six parallel jobs, which reported "refers to unregistered domain signal" before the fix.
- LT Modern: `FakeLtPeer` serves `jsonrpc-http` and runs concurrent requests last-first, as the digiBOX did on 2026-08-14. The new tests cover a subscribe right after a signal appears, an unsubscribe and a subscribe within one round trip, and the fetch subscription ending at publication.
- ws-streaming: a fake JSON-RPC HTTP server that drops, stalls and rejects requests and records their arrival order; `close()` with a stalled request returns at once.
- Hardware: `lt_race` against a digiBOX-WT; every signal keeps receiving data and closing returns in under 1 s.
