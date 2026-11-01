# CD command evidence and service boundary

`CdCommands` models the original script-facing state changes and wait requests. A missing provider is an unavailable device: play/stop do not raise a script error, and no playback clock is invented. This does not claim optical playback support on macOS. `cd_media.cpp` separately enumerates read-only IOKit media/TOC data; enumeration alone does not provide audio playback.

Evidence is the verified original executable and canonical `analysis/original-engine/instructions2/CDCore*.json`; reproducible byte ranges are included by `tools/extract_services_evidence.py`. No translated executable or patch is used.

| Entry | Observed native behavior |
|---|---|
| CD, 4375a0 | Exactly one or two arguments; otherwise null. Store script track at context+54; optional loop is true only for integer 1 at +58. Nonzero track calls 401f00; zero calls stop. The 4338f0 predicate selects immediate stop, including stop before a nonzero request. Helper return values are discarded. |
| Play helper, 401f00 | If track equals device object+2c, return without changing the worker loop or signalling. Otherwise update track and global loop 4cc064, reset global event, signal object+28, wait up to 1000 ms. Only `WAIT_TIMEOUT` (102h) writes `timeGetTime()` to both 4cbf5c and 4cc068. AL is always 1, including other wait failures. |
| Stop helper, 401f70 | Clear device track, reset/signal events and wait 1000 ms; does not clear either clock epoch. AL is always 1. Immediate helper 4021f0 additionally clears global 4cc060 before calling stop. |
| Clock helper, 401fc0 | **No MCI position query.** Clock 1 uses epoch 4cc068; clock 2 uses 4cbf5c. Zero epoch yields zero; otherwise return `timeGetTime()-epoch` with unsigned 32-bit wrap. Other clock indices yield zero. |
| CDWait, 43e830 | Positive target only. Compare unsigned clock-2 elapsed against target. If below target, create a full-window click selection and wait `target-elapsed` milliseconds. No device-existence check. With a zero epoch a positive target therefore waits its full duration. |
| CDPlayMode, 437690 / 401fb0 | Set context loop and worker loop to `argument == 1`; worker mode setter only writes the global. |
| CDPlayTime, 4376e0 | Immediately return null; it does not return elapsed time. |
| CDSilentSet, 43a380 / 402000 | No arguments: no change. Otherwise bool is `argument != 0`. True signals the device worker. False restarts only when device track is positive and worker loop equals 1; force track to zero first so the same-track suppression does not prevent restart. Other cases call stop. |
| CDSilentGet, 43a690 | Return integer bool from object+30, testing equality to 1. |
| CDNameSet, 437570 / 402050 | Only first call attempts work; it consumes its gate even when DAT56c358 disables discovery. Scan optical-drive display names, match requested case-insensitive prefix, store first matching root path. No matching media causes return with no error. |

401e10 performs MCI open/stop/close and returns false on any MCI failure. Its caller 401eb0 ignores that result. Thus the audited script/control paths propagate neither a MCI failure nor an event wait failure as a script exception. These bodies do not establish every asynchronous worker transition or the content of global 4cc060; the provider's immediate-stop operation retains that distinction rather than guessing its meaning.

Provider integration: delegate `handle()` before the legacy `AudioCommands` CD branches. `CdPolicy.immediate_stop` comes from the original skip predicate; `name_discovery_allowed` comes from the engine configuration gate. Translate a returned `wait_ms` into a click-interruptible timer and resume the VM only when the wait completes or input interrupts it. Play acknowledgement is `Signalled`, `Timeout`, or `Failed`; it is separate from an optical-device error. A provider that emulates the original worker may report epochs with `set_clock_epochs()`. `elapsed()` must keep its wall-clock semantics, even if a hardware provider also knows media position. `state()/restore()` expose script/device track, both loop modes, silent flag, one-shot name gate, and both epochs for host persistence.

The provider `find_volume` is responsible for actual case-insensitive prefix matching and retaining a selected root/device. The default unavailable provider does not claim a matched disc or advancing clock. Backend programming/I/O exceptions are not blanket-caught; ordinary absent-device/MCI errors should use the explicit acknowledgement or discovery result.

Tests in `tests/cd_commands_tests.cpp` use a deterministic clock and injected provider. They cover unavailable device, null CDPlayTime, independent modes, same-track suppression, silent loop restart, nonloop stop, failure versus timeout epoch changes, wraparound, target comparison, immediate restart, one-shot discovery, argument errors, and state restore. Direct ASan/UBSan build and run passed; no optical hardware was required or written.

## macOS mounted CDDA provider

`MacCdPlayer` supplies real `CdProvider` callbacks without sharing the main game's `Audio` object. `getmntinfo(MNT_NOWAIT)` selects only `cddafs` mounts; it enumerates regular `.aiff`, `.aif`, and `.cdda` files with leading track numbers 1–99, preserving physical numbers and rejecting duplicate numbers. If IOKit provides a TOC for the same BSD device, it excludes missing or non-audio tracks. A TOC discovery failure does not veto an otherwise readable mounted CDDA source. `CDNameSet` matches the mount basename by an ASCII case-insensitive prefix; it does not silently switch from a selected, removed volume to another disc.

Playback opens the chosen virtual track as a read-only ResourceSource, validates its header and initial PCM through the libsndfile-backed worker, and only then lazily creates a separate Audio instance. Both encoded input and decoded PCM now stream through bounded reads/queues; the old 256 MiB whole-source buffer limit is removed. Injected byte-reader hooks remain supported for tests. See [streaming pipeline](resource-streaming.md). An unfamiliar `.cdda` representation that libsndfile cannot decode is a read/decode failure, not simulated success. A failed mount/source/decode/device request returns `CdAcknowledgement::Failed`, stores an explanatory `last_error()`, and does not seed either epoch. The missing-disc path constructs no `Audio`, initializes no SDL audio subsystem, and opens no device.

A successfully started stream returns `Signalled` and invokes an explicit clock sink. Wire it as follows, keeping `player` alive longer than `cd`:

```cpp
MacCdPlayer player;
CdCommands cd(player.provider());
player.set_clock_sink([&](uint32_t track, uint32_t playback) {
    cd.set_clock_epochs(track, playback);
});
player.set_muted(mute);
player.set_silent_query([&] { return cd.state().silent; });
```

Stop and silence stop the CD stream. Bind the silent query as above so an intervening new track request cannot bypass silence; a silenced request returns Failed without seeding clocks. `CdCommands` retains the requested track/mode and invokes play again for the evidenced looping unmute/restart case. `CDPlayMode` updates the loaded stream's loop flag immediately. The provider's independent audio object and channel never enter the main `Audio::snapshot()`. Mount identity, callbacks and hardware handles are not claimed to be native save-state data.

`tests/mac_cd_player_tests.cpp` injects mounts, source reading, an offline `Audio(false)` factory and a deterministic clock. A constructed AIFF genuinely decodes to nonzero PCM; tests cover loop changes, silent restart, stop, corrupt/missing/removed sources, real default filesystem track scanning/reading, clock seeding only after success, and lazy device allocation. Ordinary and UBSan runs passed. The actual macOS mount scan on this machine found no `cddafs` disc; the real no-disc provider returned Failed without creating an audio object. **No physical CDDA playback was tested.** The ASan-linked macOS binary stalled during system-service initialization and was terminated; this is not reported as an ASan pass.
