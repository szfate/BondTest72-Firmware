# DUT Pad Map — [Project Name]

Copy this file to `DUT_PADMAP_<PROJECT>.md` and fill in every blank field.
When complete, translate into a `TestCase` array in `src/test/pad_map_registry.cpp`.
Use `ADAPTER_MEZ70.md` to convert adapter pins to tester channels.

Adapter: [adapter name]
Source: [spreadsheet / schematic reference]

Die pad 0 is at the top-right corner; numbers increase counter-clockwise.

**Mapping layers:**

| Layer | Formula | Owner |
|-------|---------|-------|
| Die Pad → DUT Pin | physical bond wires on the DUT PCB | this die |
| DUT Pin → Adapter Pin | `adapter_pin = 71 − dut_pin` | DUT PCB (connector orientation) |
| Adapter Pin → Tester Ch | `tester_ch = adapter_pin − 1` | adapter — see `ADAPTER_MEZ70.md` |

> **DUT Pin is reference only.** It traces the physical bond wire path and is
> not used in firmware. Only Adapter Pin values appear in `pad_map_registry.cpp`.
> See `GLOSSARY.md` for definitions.

---

## Identity

| Field | Value |
|-------|-------|
| `id` (ProjectId enum value) | |
| `name` (short human-readable string) | |

---

## Die Pad to Adapter Pin Cross-Reference

| Die Pad | Signal | DUT Pin | Adapter Pin | Role |
|---------|--------|---------|---------|------|
| 0  | | | | |
| 1  | | | | |
| …  | | | | |

---

## GND Pads

Multiple GND pads are typically present. Pick one for current injection; note the others.

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| | | | |

GND plane only (no DUT pin, no adapter pin): die pads [list if any]

```
gndPin =   // adapter pin — TBD after hardware bring-up
```

---

## IO Adapter Pins in Ring Order

Counter-clockwise from die pad 0 (top-right corner), IO connections only.
Values are **adapter pins** (1-indexed). GND/VCC die pads are skipped.

```
ioAdapterPinsInRingOrder = [

]
```

---

## Presence Detection

Pick two GND adapter pins that are always connected on this DUT. The adapter
PCB shorts them together — firmware reads continuity to detect DUT presence.

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| | | | |
| | | | |

```
presenceThresholdV = 0.3  // default — calibrate in Phase 3 if needed
```

---

## Test Thresholds

Leave blank until Phase 3 hardware calibration. Defaults from ARCHITECTURE.md
are shown; override here if this die's process differs.

| Signal | Threshold | Default | This die |
|--------|-----------|---------|----------|
| COM_D good bond min | V | 0.50 V | |
| COM_D good bond max | V | 0.70 V | |
| COM_D open bond min | V | 2.50 V | |
| COM_D short max | V | 0.10 V | |
| COM_A / COM_C short max | V | 0.30 V | |

---

## Notes

<!-- IO count, power domains, any die-specific quirks or bonding exceptions -->
