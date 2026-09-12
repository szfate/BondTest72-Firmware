# BondTest72 Code Review

*2026-09-11 · all 55 src files and 10 tool scripts read in full at HEAD `f7ef512`.*

**Progress 2026-09-12:** done today — B1–B8, all three cheap-hardening items, C1–C9, F2, F3 (struck through below). Notes: B6 was fixed differently than prescribed (adapters are *not* trivially destructible — `AdapterBase` has a virtual dtor — so `create()` now ends the previous occupant's lifetime explicitly instead of asserting). Also done beyond the review: adapter-insertion settle+retry (no more spurious NOT_PROVISIONED/FAULT on seating) and EEPROM blank-header re-read confirmation; pullups retuned to 280k/27.4k/2.49k from bench IV data (supersedes the 33k/27k references in C2/C3).

Items are grouped by **effort to fix** (S = minutes, M = an hour or two, L = half day+). Bugs first, then improvements.

---

## P0 — Real bugs (fix before next bench session)

| ID | Where | What | Fix | Effort |
|----|-------|------|-----|--------|
| ~~B1~~ |  `src/app/host_protocol.cpp:53-80` | `PROVISION` without a `padmap=` key is silently dropped (returns `NONE`, no response to host). Protocol doc (`docs/BONDTEST72_HOST_PROTOCOL.md:103`) promises `ERROR code=6 msg=MISSING_PADMAP`. The check in `state_machine.cpp:208-209` is unreachable dead code. | Track key-presence per field: return a `PROVISION_INVALID` command instead of `NONE` so the state machine emits the promised error. Proper fix is a `ProvisionRequest` struct (see F4). | S |
| ~~B2~~ |  `src/app/state_machine.cpp:332-338` | Boot-time EOL adapter (`begin()` :47-48) bypasses `transition()` and goes straight to `State::EOL_ADAPTER` — no `EVENT EOL_WARNING` is sent to the host, and the SK6812 status LEDs never light. The runtime path (:109-113) does both correctly. | Route the boot path through `transition()` (extract a shared `handleAdapterArrival()` — see F1). | S |
| ~~B3~~ |  `src/app/state_machine.cpp:403-417` | `GET_RESULTS` iterates the **live** `_padMap` but `_lastResult` was captured under the map active at `startTest()`. A `SET_PADMAP` between run and re-fetch mislabels every PAD line's die-pad numbers. | Cache `const PadMap* _lastResultPadMap` in `startTest()`; use it in `sendResults()`. | S |
| ~~B4~~ |  `src/test/pad_map_registry.cpp:21` | Threshold comment rot on the **GOOD/OPEN decision boundary**: comment says 50 kΩ with margin arithmetic tuned for 50 k, but the constant is **60 000 Ω**. Any future reader "fixing" the comment could silently change the threshold. | Author confirms 60 kΩ against bench data; then reconcile the three comment sites (registry :12-20, pad_map.h:34-35, kelvin.h comment) with corrected margins (>6× above the strongest real-bond reading, >4× below the weakest unbonded artifact). | S |
| ~~B5~~ |  `src/hal/kelvin.cpp:75,96` | Drain-settle is hardcoded `1000 µs` in two places with a comment saying "scale with padmap's largest cap" — but it's not parameterized. Any padmap with a cap >5 µF silently under-drains. | Extract `DRAIN_SETTLE_US = 1000` (named constant only; do NOT restructure the sequencing — it's latchup-safety-critical). Longer term: derive from `max(tc.settleUs)` for CAP_SENSE cases. | S |
| ~~B6~~ |  `src/adapter/adapter_registry.cpp:16-18` | `create()` placement-news **over a live object** on every adapter re-detection (state_machine.cpp:305). Safe today only because `Mezzanine70` is trivially destructible and callers null the pointer first — a nontrivial adapter would be UB. | Add `static_assert(std::is_trivially_destructible_v<Mezzanine70>)` and `std::is_base_of_v<AdapterBase, Mezzanine70>`; fix the "Call once at boot" comment (it isn't — it's called on every re-detection). | S |
| ~~B7~~ |  `src/adapter/eeprom_manager.cpp:16-18` | SWI transport failure (`_eeprom.read` → false) is returned as `ReadResult::CrcError`, conflating a flaky 1-Wire contact with a genuinely corrupt header. Harder to debug on the bench. | Add `ReadResult::IoError`; callers already treat both as not-Ok, so zero behavior change. | S |
| ~~B8~~ |  `src/debug/eeprom_test.cpp:9-10` | Scratch-area comment claims "bytes 24-127" but the header+CRC occupy **0-35**. Anyone "correcting" the test downward would clobber `testCount` / `eolReached`. Also auto-provisions with defaults that drifted from the real `PROVISION` path. | Fix comment (scratch starts at byte 36). Longer term, delete this file (see F8). | S |

### Cheap hardening (same phase as P0)

- ~~**Untested channels read as GOOD.**~~ DONE: `BondResult::NOT_TESTED = 0` added; GOOD/OPEN shifted down; wire format unchanged.
- ~~**`parseKvUintList` trailing comma** (`host_protocol.cpp:113`)~~ DONE: empty fields (trailing/doubled comma) rejected → whole list fails → `PROVISION_INVALID` → `ERROR MISSING_PADMAP`.
- ~~**`_lastAdapterPoll`** (`state_machine.h:65`) serves two cadences~~ DONE: split into `_lastAdapterLivePoll` (1500 ms) / `_lastAdapterInsertPoll` (100 ms).

---

## P1 — Comment rot on safety-adjacent code (trivial, high value)

| ID | Where | What |
|----|-------|------|
| ~~C1~~ |  `src/hal/mux.cpp:21-23` | TODO proposes swapping `digitalWrite` for `gpio_set_mask` as a "50x speedup". That would invalidate the latchup exposure-window analysis in `kelvin.cpp:48-57`. **Rewrite as a warning**, not a TODO. |
| ~~C2~~ |  `src/adapter/mezzanine70.cpp:49-50` | "27K pull-up" → actually 33 k (`PULLUP_LEVELS[1]`); "Read COM_D" → actually COM_A. |
| ~~C3~~ |  `docs/MUX_MAP.md:21` | "27 kΩ pullup" stale vs code (33 k). |
| ~~C4~~ |  `src/test/pad_map.h:6-7` | Says `PULLUP_LEVELS` lives in `test_runner.cpp`; it's in `hal/kelvin.cpp:6-10`. |
| ~~C5~~ |  `src/adapter/mezzanine70.cpp:24` | Logs `caseCount` as "%u pads" — it counts test *steps* (incl. DISCHARGE), not pads. |
| ~~C6~~ |  `src/adapter/mezzanine70.cpp:49-52` | `DIODE_ANODE`/`DIODE_CATHODE` are tester channels (70/71), not adapter pins. Rename to `..._CH` so nobody adds the (wrong) `channelForPin()` call. |
| ~~C7~~ |  `src/adapter/eeprom_layout.h:20` | Comment mentions `EEPROM_WIRE_BYTES` (constant is `EepromData::WIRE_BYTES`). |
| ~~C8~~ |  `docs/ADAPTER_MEZ70.md:13` | References `TestCase.mezPin`; field is now `adapterPin`. |
| ~~C9~~ |  `src/hal/buttons.cpp:4` | Says "internal pullup" but code is `INPUT` (external pullup fitted; RP2350 errata precaution). Doc-only fix. |

---

## P2 — Duplication (the big theme)

| ID | Where | Fix | Effort |
|----|-------|-----|--------|
| ~~F1~~ | `state_machine.cpp:43-58` vs `:104-123` | DONE: `handleAdapterArrival()` extracted; both boot and runtime insertion poll route through it (and through `transition()`). Boot `prime()` and transient-retry remain at their call sites. Fixes B2. | M |
| ~~F2~~ |  `state_machine.cpp:93-102` vs `:182-188` | Button-start and host-RUN-start are identical. Extract `tryStartTest()`. | S |
| ~~F3~~ |  `host_protocol.cpp:141-149`, `:163-171`, `:196-204` | Padmap-list printing ×3. Extract `printPadmapList()`. | S |
| F4 | `host_protocol.cpp:53-80` + `state_machine.cpp:203-228` + `eeprom_test.cpp:45-52` | Provision defaults ×3 (already drifted), blank-detection ×2. Hoist into `eeprom_layout`: `PADMAP_ID_SLOTS`, `eepromHeaderLooksBlank()`, `eepromDefaults(hw)`. Replaces 7 scattered raw sentinels with a `ProvisionRequest` struct. Properly fixes B1. | M |
| ~~F5~~ | `test_runner.cpp:138-152` vs `state_machine.cpp:409-414` | DONE: verify loop folded into the measurement loop (FAIL set at the first non-GOOD pad; FAIL_DUT_REMOVED later still overwrites, outcome precedence unchanged). `forEachTestedCase()` skipped — with the verify loop gone only `sendResults()` walks that shape, so the helper would be one-consumer indirection, not dedup. | M |
| ~~F6~~ | `dut_detector.cpp:28-32` vs `:44-49` | DONE: private `senseCandidate()` extracted; `prime()` and `poll()` both use it. | S |
| ~~F7~~ | `mezzanine70.cpp:93-110` | DONE: private `kelvinPresence()` helper extracted; `senseDutPresent`/`senseDutFlipped` are now one-liners mirroring it (LOG_D tags preserved). | S |
| ~~F8~~ | `src/debug/` | DONE: mux_waveform_test, button_test, adc_test, sk6812_test, eeprom_test deleted (10 files); only adapterSelfTest and log remain. Also removes main.cpp's unused `debug/eeprom_test.h` include (LY4). Resolves B8's comment rot and C9's doc drift. | S |
| ~~F9~~ | `mux_map.h:9` + `mux_map.cpp:5` + `mux.h:4` | DONE: `MUX_MAP` sized by `MUX_CHANNEL_COUNT` (mux_map.h now includes mux.h); static_assert guards the table's 72-entry assumption. | S |
| ~~F10~~ | `result.h:40-54` + `test_runner.cpp:46,51,73` + `host_protocol.cpp:313,322` | DONE: went further than the original helpers — `PadResult` now has separate `fwd[]`/`rev[]` groups (same memory), so the strategy-dependent base index vanished entirely; only `readingsPerDir()` remains as the group-length rule. | S |
| ~~F11~~ | `state_machine.cpp:225` | DONE: `HostProtocol::sendOk(what)` added; PROVISION's `OK PROVISION` now routes through it (wire byte-identical — doc + provision.py match the exact string). | S |
| ~~F12~~ | `state_machine.h:62` vs `host_protocol.h:80` | DONE: `_adapterUid` dropped from StateMachine (was a third buffer + duplicate fallback); `setAdapterUid(nullptr)` fallback is the single source. HostProtocol's `_uid` → `_adapterUid` + `setAdapterUid()` for clarity vs the tester's HELLO `uid=` (RP2350 OTP board ID, sent live, never stored). Doc: `aid=` description now states both IDs and the '0' fallback. | S |
| ~~F13~~ | `kelvin.cpp:4` vs `adc.cpp:8` | DONE — with one deviation: `VCC` lives in `adc.h` instead of kelvin.h (kelvin.h already includes adc.h, and putting it in kelvin.h would force adc.cpp to pull kelvin.h's test-layer includes — the LY1 trap). One shared `inline constexpr float VCC = 3.3f`. | S |

---

## P3 — Magic numbers → named constants (all S, timing-neutral)

- ~~**N1**~~ DONE: `enum class AdcChannel {ComD, KelvinSense, ComC}` in adc.h; `readVoltage(AdcChannel)`; call sites use the enum.
- ~~**N2**~~ WONTFIX (user): index names don't scale to 4/6 future pullup levels — `PULLUP_LEVELS[1]` / `COUNT-1` stay, ordering documented in pad_map.h.
- ~~**N3**~~ DONE: `SELF_TEST_SETTLE_US` (mezzanine70.cpp), `IO_SETTLE_US`/`CAP_SETTLE_US` (pad_map_registry.cpp).
- ~~**N4**~~ DONE earlier (B5).
- ~~**N5**~~ DONE: `READ_ATTEMPTS = 3` in at21cs01.cpp (readSerial stays single-shot — separate hygiene item).
- ~~**N6**~~ DONE: `BOOT_BRIGHTNESS`/`BOOT_STEP_MS`/`BOOT_WHITE_MS`/`BOOT_WHITE_LEVEL` in sk6812.cpp.
- ~~**N7**~~ DONE: `Y4_BASE`/`Y4_GROUP_STRIDE`/`Y4_GROUP_SIZE` in mux.cpp.
- ~~**N8**~~ DONE: `LINE_OVERFLOW_WARN_LEN`; prefix lengths via `sizeof("...") - 1`; sentinels `EepromData::HWID_UNSET`/`FIELD_UNSET` (eeprom_layout.h) used across host_protocol/state_machine. Blank-flash check in eeprom_manager stays raw `0xFF` (physical erased state, deliberate).

---

## P4 — Layering (optional but cheap wins)

- **LY1** `kelvin.h` includes `test/pad_map.h` + `test/result.h` — HAL depends upward on its caller. Caused C4's doc drift. Moving measurement policy into `test/` is the clean fix (L); a `void` and one comment fix is the pragmatic minimum.
- ~~**LY2**~~ MOOT: `DiscoveryScanner` removed entirely (user decision — hardware is stable, bring-up instrument no longer needed). The app→test inversion died with it.
- **LY3** `main.cpp:14-24` globals with external linkage → anonymous namespace (`mux`/`adc` are collision magnets).
- **LY4** `main.cpp:12` includes `debug/eeprom_test.h` — unused after F8.
- ~~**LY6**~~ MOOT: `DiscoveryScanner` removed (same decision as LY2).
- **LY7** `state_machine.h` includes 12 headers for 8 ref members → forward-declare.
- **LY8** `getSupportedPadmapIds()` returns a length-less pointer; `4` hardcoded at `state_machine.cpp:332` → `const uint8_t (&)[4]`.
- **LY9** `DUT_POLL_INTERVAL_MS` lives in `dut_detector.h`, consumed only by `state_machine` → move to app.

---

## P5 — Structural (M effort each, highest payoff first)

| ID | Fix | Impact |
|----|-----|--------|
| ~~**S1**~~ | DONE (note: TestResult is ~8.9 KB since MAX_DUT_SLOTS=1, not 44 KB). `run(adapter, padMap, TestResult& out)` writes into caller-owned storage — no stack frame, no by-value copy. | Kills the whole-struct stack frame + copy. Risk: none (caller already holds a static). |
| ~~**S2**~~ | DONE: `IO1`/`IO2` merged into single `IO(...)` (byte-identical bodies). Optional `CS(...)` shorthand for CAP rows skipped — rows carry per-case padType/comments, shorthand saves little. | -~10 lines, removes a drift trap. |
| ~~**S3**~~ | DONE: `printBondThreshold()` / `printCapSchedule()` extracted; header line converted to `Serial.printf` (integer fields; float fields kept on `Serial.print(x, 0)`). Wire output byte-identical. | Readability; no behavior change. |
| ~~**S4**~~ | DONE: per-state `CMD_MASK[State::COUNT]` gates `handleCommand()` up front; rejections get new `ERROR 8 WRONG_STATE` (msg = state name). **Spec table was itself wrong and was reconciled against real tool usage**: PROVISION is valid everywhere except TESTING (fresh blank adapters latch FAULT — provision.py provisions from there), GET_ADAPTER/HELLO valid everywhere (provision.py + live_serial_viewer.py query from any state). GET_RESULTS also allowed in ADAPTER_DETECTED (results stay valid after DUT removal, user-approved); re-provisioning allowed in ADAPTER_DETECTED/READY (user decision). SET_PADMAP gate closes B3's root cause. | Host-visible: out-of-spec commands now answered with WRONG_STATE instead of executing. |
| ~~**S5**~~ | DONE: pure code motion into `pollAdapterLiveness()` / `pollDut()` / `pollAdapterInsertion()`; `update()` is now 10 lines of named jobs. Start-button consumption stays in `update()` deliberately — `startPressed()` is a consume-on-read latch, and folding it into pollDut's state gate would let a press during TESTING linger and fire after PASS. | Readability. |
| ~~**S6**~~ | DONE: `LedManager` renders into a 3-pixel pattern cache and only pushes `clear()+show()` when it differs (`_rendered` sentinel forces the first render). Blink phase still derives from `millis()` → redraws only at blink edges. NO_ADAPTER/EOL dim-red now via 3× `setPixel` (was `setAll` — visually identical). **Bench-verify pending** — changes hal-call interleaving. | Frees IRQ headroom (~340 µs → 0 µs between blink edges). |
| ~~**S7**~~ | DONE: invalid channel returns `NAN` (never taken today — enum-typed `AdcChannel` makes it hard to hit; safety default). Verified fail-safe through `classifyVoltage`: NaN comparisons are false → resistance clamps to 1e9 → conducted=false → OPEN, never a false GOOD. | Safety default. |
| ~~**S8**~~ | WONTFIX (user): no version byte now. Decision: `buf[3]` rfu is the earmarked slot for `layout_version` once a second header layout actually exists (notes added in eeprom_layout.h, README, ARCHITECTURE) — not the review's proposed `buf[8..11]`, which a future layout would consume first. No code change. | Future-proofing via documentation. |

---

## Small hygiene (batch opportunistically)

- `at21cs01.cpp`: `readSerial` no retry while `read` retries ×3 — comment if deliberate or unify.
- `eeprom_manager.cpp:20-21`: hexdumps 8 bytes at INFO on every insertion → LOG_D.
- `led_manager.cpp`: `blinkOn()` can be static; `stateName()` could live in `state.h`.
- `host_protocol.h`: declaration alignment drift (:43-46); no `sendOk()`.
- Forward-decl instead of includes: `adapter_registry.h:3`, `mezzanine70.h:3`, `eeprom_manager.h:3`. `log.h` should include `Arduino.h` itself.
- `Bus` enum values are Y-port indices, unvalidated — comment.
- `mezzanine70.h:11` hardcodes `70` alongside `ADAPTER_PIN_COUNT_PLUS_1 = 71` — same fact, two constants.
- `flushEeprom` reuses `PROVISION_FAILED` for runtime write failures → distinct error code (needs doc table row).
- `prot doc`: note logs and protocol share one CDC channel — host must tolerate interleaved `[t] [INFO]` lines.

---

## Deliberate design — do NOT churn

1. **Latchup mitigation sequencing** everywhere: ground-reference-first, drain-before-release, reverse-only `MEASURE_DIRECTIONS`. All refactors preserve these.
2. **AT21CS01 bit-bang timing**: `delayCyc`/`cyc` bodies, DWT macros, bounded `noInterrupts()`. Naming-only changes.
3. `mux.clearAll()` belt-and-braces in `mezzanine70.cpp:35` — redundant but intentional.
4. `selfTest` passing raw 70/71 without `channelForPin` is **correct** (tester channels, not adapter pins).
5. `start()`/`stop()` identical in at21cs01 = semantic protocol concepts.
6. Blocking synchronous test run; DUT-removal via `FAIL_DUT_REMOVED`.
7. `MAX_DUT_SLOTS = 5` headroom is deliberate — deserves a comment, not a shrink.
8. Wire-format rationale comments in `host_protocol.cpp` stay verbatim through any refactor.
9. Bitwise CRC-32 loop is fine at boot frequency.
10. `measureKelvinCurve` doesn't compensate ADC jitter — known, documented approximation.

---

## Suggested phasing

### Phase 0 — bugs + comment rot + dead files (≈1 hour)
| # | Change |
|---|--------|
| 0.1 | B1: PROVISION without `padmap=` → emit `ERROR code=6` (add key-presence tracking) |
| 0.2 | B2: boot EOL → route through `transition()` |
| 0.3 | B3: cache `_lastResultPadMap` |
| 0.4 | B4: reconcile threshold comments to 60 kΩ |
| 0.5 | B5: extract `DRAIN_SETTLE_US` |
| 0.6 | B6: registry static_asserts + lifecycle comment |
| 0.7 | B7 + F8: delete 5 dead debug tests (10 files) |
| 0.8 | C1–C9: comment rot fixes |
| 0.9 | `pio run` — compile gate |

### Phase 1 — mechanical dedup + constants (≈2 hours)
Dedup F2/F3/F6/F7/F9/F10/F11/F12/F13; named constants N1–N8; hardening (NOT_TESTED, parseKvUintList, registry asserts); `pio run`.

### Phase 2 — structural (≈ half day)
S1 → F5 → S2 → S3 → S4 → S5 → S6 → S7 → `pio run` + bench smoke test (adapter detect, DUT insert/remove/flip, one full test, PROVISION on scratch).

### Explicitly not doing
Kelvin layering (user decision), debug harness (files deleted instead), threshold value change (60 k stays), MeasurementCtx (when next adapter lands), EEPROM version byte (future).

---

## Verification limits

No unit-test infrastructure. Gate = `pio run` clean per phase. Behavior needs a bench smoke test at the end.
