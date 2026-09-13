# Improvements & Public-Release Roadmap

A parking lot for everything that should happen before (or shortly after) the public
release — hardware, firmware, host software, UX, docs, and community infrastructure.
Items here are ideas, not commitments; promote them to tracked issues when work starts.
Cross-references to `code_review.md` use its finding IDs.

---

## 1. Hardware — Reference / Calibration DUT Board

**Idea:** a reference DUT board (an adapter with a passive "die" on it) that lets the
operator verify the *tester itself* — detect measurement drift, mux channel
degradation, and contact wear without needing a known-good real die.

**References:**
- **Open — skip the board.** An empty socket already *is* the open reference; the
  firmware classifies opens today. Nothing to build.
- **Short (0 Ω):** direct trace (or jumpered) pad-to-GND shorts at a few pads.
  Verifies the low-resistance measurement path end-to-end: mux R<sub>on</sub>, contact
  resistance, and the Kelvin sense chain.
- **Load:** precision resistors at selected pads — e.g. 1 kΩ, 10 kΩ, and one close to
  the ceiling (≈51 kΩ, margin below the 60 kΩ classification threshold). Verifies
  classification on both sides of GOOD/OPEN and exercises all three pullup levels.
  Reuse the 1 kΩ precision resistor type from the Mezzanine70 r2 self-test for
  consistency.

**Design considerations:**
- Trace + connector series resistance adds to apparent R. Budget it: keep short
  traces on the short references, and choose load values with margin so a few ohms
  of path resistance can't flip a classification near 60 kΩ.
- Spread references across **all three CH446X chips** and **both GND groups**
  (GND and GND3 returns) — a reference that only exercises chip 0 proves nothing
  about chips 1–2.
- Temperature-stable resistor type (≤25 ppm/°C) for the load references; a cheap
  1% resistor drifts enough to matter near the ceiling.
- Give the board its own pad-map ID (or extend an existing map) so the firmware
  recognizes it and can run a dedicated check sequence.

**Firmware support (see §3):** a `CAL_CHECK` command that runs the reference board,
compares measured vs. expected resistance with a documented pass band, and logs the
result — enabling drift tracking over the tester's lifetime (compare against
insertion/test counters). Also closes the loop on review finding **M8** (self-test
result computed and discarded).

---

## 2. Public-User Adapter & Padmap Model — the big one

**The factory model doesn't transfer.** Factory testers pair one adapter with one
padmap: the adapter's EEPROM declares its padmap IDs and the connector mechanically
matches the die. Public users will realistically own **one adapter** and need to run
**multiple padmaps** — and, in wafer.space-style flows, eventually padmaps for dies
that don't exist yet.

**What already exists:** `SET_PADMAP id=` override, a universal fallback map, and a
per-adapter padmap whitelist in EEPROM. The firmware skeleton is there; the product
around it isn't.

**Options, small to large:**
- **A. GUI padmap picker (medium).** New `LIST_PADMAPS` protocol command returning
  registered IDs + descriptors (die name, pad count, required GND pins); GUI
  dropdown filtered to maps *compatible with the connected adapter*. Harden
  `SET_PADMAP` per review m13/m14 (range check — no `(uint8_t)` wrap — and an
  error reply instead of silence).
- **B. User-configured adapter whitelist (small–medium).** The included adapter is
  configured for the specific padmaps its owner uses: the GUI writes the padmap ID
  set into the adapter's EEPROM (via PROVISION, not raw text). Keeps the safety
  property that the *adapter declares what it can run*; auto-selection then just
  works. Depends on R3 (PROVISION validation) landing first.
- **C. Upload custom padmaps from host (big development).** For customers testing
  their own die: a padmap upload protocol, persistent storage (flash/LittleFS),
  validation (pin bounds, GND validity, strategy sanity), versioning, and a GUI
  die-map editor (reusing the `die_visualizer` drawing code). This is a project,
  not a feature — schedule as its own milestone. Until it ships, new padmaps
  arrive via firmware release.

**Safety note:** running an incompatible padmap can drive VDD pads as IO and
misclassify results. Whatever the option, the GUI must filter by compatibility, the
firmware must keep a working fallback, and the universal-map fallback must be
documented behavior rather than a silent surprise.

---

## 3. Firmware

**Correctness / robustness** (mostly from `code_review.md`, release blockers first):
- [x] R2 (minimum fix) — EEPROM write-verify + one retry in `EepromManager::write()`;
      persistent flush failure escalates to `EVENT FAULT` + FAULT state. Dual-record
      layout for torn-write recovery still open (see `code_review.md` R2 "proper" fix).
- [ ] R3 — strict PROVISION parsing: numeric validation, `hw` registry check,
      lifespan/date/eol range checks, host-visible rejection.
- [ ] R4 — sticky line-overflow flag; reply `ERROR … LINE_TOO_LONG` instead of
      executing truncated lines.
- [ ] M6 — incremental test runner (or interleave host-poll/liveness between pads);
      emit the already-documented `BUSY` error mid-test; detect adapter removal
      mid-test instead of reporting results against a disconnected board.
- [ ] M7 — boot-time transient EEPROM failure should retry, not latch FAULT.
- [ ] M8 — act on the adapter self-test result (FAULT/EVENT on FAIL) or strip it
      from production builds.
- [ ] Watchdog in `main.cpp` (review m16 note): a hard fault currently hangs the
      tester silently.
- [ ] m1 — clamp negative Kelvin voltage in `classifyVoltage` (one line, removes
      a latent false-GOOD path).
- [ ] m10 — close the 60 kΩ ceiling calibration debt at the current pullup values;
      link bench data.
- [ ] m17 — ship production builds with errors-only logging (protocol wire stays
      clean for third-party host software).

**Features:**
- [ ] `LIST_PADMAPS` host command (IDs + descriptors) to back the §2-A GUI
      picker; harden `SET_PADMAP` per m13/m14.
- [ ] `CAL_CHECK` host command for the reference board (see §1), with pass bands
      and result logging.
- [ ] Optional per-unit calibration constants if golden-board measurements reveal
      a systematic offset worth compensating (the ratiometric math already cancels
      rail tolerance, so this should be rare — decide with data).
- [ ] Test-progress feedback on the LEDs (e.g. sweep animation during TESTING)
      — cheap on-device UX win, see §3.

---

## 4. Host Software, GUI & UX

The GUI already lives in `tools/` — `die_visualizer/` (die-map rendering, log
parsing) and `live_serial_viewer.py`. The work below is about turning these into
the product a customer experiences, with §2 as the driving requirement.

- [ ] **Single entry-point application** built on the existing widgets:
      auto-detect port, show adapter info/EOL status, trigger tests, die-map
      visualization with per-pad measured resistance, pass/fail summary.
- [ ] **Padmap picker** per §2-A: `LIST_PADMAPS` + compatibility-filtered
      selection; option B (adapter whitelist config) as a settings action.
- [ ] **Result history & traceability:** CSV/JSON export, record per-test
      (adapter ID, insertion count, timestamp, per-pad results) — production
      customers will want die-serial → result mapping.
- [ ] **Firmware update flow** in the GUI (UF2 drag-drop works, but wrap it).
- [ ] **On-device UX pass:** LED behavior during testing (progress indication),
      consider what a red/green result should do after DUT removal (today results
      hold until next insertion — confirm this matches operator expectations),
      document button gestures if any are added.
- [ ] First-run experience: what does a customer see when they plug it in with no
      adapter? (Currently dim red LEDs + serial silence — probably fine, but worth
      a deliberate decision and a quickstart doc.)

---

## 5. Docs & Community

- [ ] User manual / quickstart (public-facing, separate from the engineering docs).
- [ ] Calibration procedure doc: how often, with what, pass bands, what to do on
      failure (pairs with §1 and `CAL_CHECK`).
- [ ] Staleness pass per `code_review.md` §4: DiscoveryScanner diagrams, LED/state
      tables in ARCHITECTURE.md, README error codes 8–9, tools README lifespan
      default, button-count and connector-count wording.
- [ ] `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, issue/PR templates.
- [ ] SPDX headers or NOTICE file; datasheet attribution note in `docs/datasheets/`.
- [ ] CI (GitHub Action running `pio run`); pin `platformio.ini`; release tags +
      `CHANGELOG.md`.

---

## 6. Manufacturing & Field Support

- [ ] Decide and document the EOL flow from the operator's perspective (what they
      see, how adapters get re-provisioned or retired; review M12 off-by-one first).
- [ ] Provision tooling polish based on factory feedback (R3 validation changes
      the failure modes here).
- [ ] RMA / field-diagnostics story: what logs, what commands, what does support
      ask the customer to run?
