# BondTest72 — What to Provide Before Coding Can Proceed

Items are ordered by dependency. Each phase unblocks the next.

---

## Phase 1 — Unblocks HAL drivers (hard blockers)

- [x] **Finalized schematic** — pin assignments captured in `docs/RP2350 PINMAP.md`
  - GP1  → SK6812 DIN
  - CH446X bit-bang serial: DAT1/CLK1/STB1 (GP13/14/12), DAT2/CLK2/STB2 (GP10/11/5), DAT3/CLK3/STB3 (GP8/9/4), RST shared (GP7)
  - GP26/ADC0 → COM_D (injection+sense, 27 kΩ pullup)
  - GP27/ADC1 → COM_A (neighbour sense, 1 MΩ divider)
  - GP28/ADC2 → COM_C (neighbour sense, 1 MΩ divider)
  - GP17 → AT21CS01 SWI (CON6)
  - GP0  → START button (active low, internal pullup)
  - GP6  → NC (spare)
  - GP15/22/18/19/20/21 → CON0–CON5 (adapter control); CON6 = EEPROM SWI
  - Still needed: which adapter-control line drives the adapter ID LED

- [x] **CH446X datasheet** — provided in English and Mandarin. Note: datasheet covers
  both CH446Q and CH446X variants; schematic uses CH446X — confirm any behavioural
  differences between variants before writing the abit-bang serial driver.

- [x] **AT21CS01 datasheet** — provided.

- [x] **MUX channel mapping table** — completed in `docs/MUX_MAP.md`.
  Chips are U2/U3/U4 (no U1 — likely the Pico module). Logical pad = CH net number.

---

## Phase 2 — Unblocks test engine

- [ ] **Pad map definitions** (×4) — for each known die project, provide:
  - Pad roles: IO / GND / VCC / NC for each of the 72 pad indices
  - Physical pad ring order (determines neighbour pairs for adjacent-bond testing)
  - GND pad index (current injection point)
  - Two pads that are shorted through the DUT PCB (for presence detection)

- [ ] **Adapter EEPROM initial values** for Mezzanine70 — needed to define the
  programming fixture / factory script:
  - `adapter_model` byte value for Mezzanine70 (e.g. `0x01`)
  - `adapter_version` for first production run
  - `supported_padmap_id` — which of the 4 pad maps this adapter carries
  - `designed_lifespan` — rated insertion count for the 70-pin 0.4 mm mezzanine connector

---

## Phase 3 — Calibration (requires assembled hardware)

- [ ] **COM_D voltage thresholds** — measure on real silicon to confirm:
  - Good bond range (expected ~0.5–0.7 V, process-dependent)
  - Open bond threshold (expected >~2.5 V)
  - Short threshold (expected <~0.1 V)

- [ ] **COM_A / COM_C neighbour sense thresholds** — measure with and without a
  deliberate neighbour short to confirm the 1 MΩ/1 MΩ divider midpoint and short
  detection threshold (expected ~1.65 V no-short, <~0.3 V short)

- [ ] **Presence detection threshold** — measure COM_D with and without DUT seated
  to confirm the continuity voltage level for the chosen presence pad pair

---

## Phase 4 — Future adapter (not needed for MVP)

- [ ] **Mezzanine70x5 GPIO mapping** — which of the 7 adapter-control GPIOs drives
  which CBT16211 isolation switch for each of the 5 DUT slots
