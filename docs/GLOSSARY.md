# Glossary

Definitions for terms used across DUT pad maps, adapter docs, and firmware.

---

## Die Pad

The physical pad on the silicon die. Numbered **0-indexed**, counter-clockwise
starting from pad 0 at the top-right corner (die orientation mark).

This is the primary identifier in DUT pad maps and the number used in firmware
for logging (`TestCase.diePad`).

---

## DUT Pin

The connector pin on the DUT PCB — the small PCB the die is bonded onto.

**Reference only. Not used in firmware.**

Present in pad maps for traceability: it shows which bond wire connects a die
pad to the adapter connector. The relationship to adapter pin is fixed by the
DUT PCB's connector orientation:

```
adapter_pin = 71 − dut_pin   (current DUT PCB design)
```

If a new DUT PCB design uses a different connector orientation, this formula
changes and a new pad map is needed. The DUT pin values themselves never appear
in `pad_map_registry.cpp`.

---

## Adapter Pin

The pin number on the adapter connector (1-indexed).

This is the number firmware uses — it appears in `TestCase.mezPin`,
`presencePadA`, `presencePadB`, and `gndPin` in `pad_map_registry.cpp`. See
the relevant adapter doc (e.g. `ADAPTER_MEZ70.md`) for the full pin-to-channel
table.

---

## Tester Channel

0-indexed firmware index into the mux driver. Derived from adapter pin:

```
tester_ch = adapter_pin − 1
```

Used internally by `channelForPin()`. Does not appear in DUT pad maps — compute
from adapter pin when needed.

---

## GND Plane Pad

A die pad bonded to the DUT PCB's GND plane rather than to a dedicated
connector pin. These pads have no DUT pin, no adapter pin, and no tester
channel. They appear in the cross-reference table with `—` in the DUT Pin and
Adapter Pin columns.
