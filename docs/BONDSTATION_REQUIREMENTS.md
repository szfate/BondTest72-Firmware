# BondStation — Software Requirements v0.3

Companion application for the BondTest72 wirebond tester.

---

## Context

BondStation runs on a Linux touch panel PC connected to a BondTest72 tester via USB. A factory line worker loads a die into the adapter, presses the physical start button, and the software records the outcome with an accompanying camera image. The tester handles pass/fail signalling via LEDs; BondStation mirrors and records.

Test runtime is under 1 second. The bottleneck is the operator, so the UI must minimise friction and cognitive load.

---

## Users

**Primary: factory line worker**
- Runs tests repeatedly throughout a shift
- Needs an unambiguous pass/fail result
- Is not expected to interpret electrical detail

**Secondary: engineer / supervisor**
- Reviews historical results and images
- Investigates failures by examining per-pad voltage data
- Configures adapter, pad map, and camera settings

---

## Functional Requirements

### 1. Connection

- Auto-detect BondTest72 on available USB serial ports at startup
- Reconnect automatically if the USB cable is unplugged and re-plugged (the device node may change — rescan ports)
- Display connection status (connected / disconnected) persistently in the UI
- Show adapter info on connection: model, version, pad map name, insertion count, test count, EOL status
- When EOL is reached, display a **blocking overlay** that prevents further testing but keeps the UI visible (adapter must be replaced). Dismissible only by replacing the adapter (EOL flag clears).

### 2. Test execution

- Test is triggered by the **physical button** on the tester (primary). A GUI trigger button is also available for diagnostic use.
- GUI mirrors tester state at all times (ADAPTER_DETECTED → READY → TESTING → PASS/FAIL)
- On completion, display **PASS** or **FAIL** prominently (full-screen colour flash or large indicator)
- Display a per-pad die map with result colours on each pad immediately after test; should show pass/fail with color, also needs to show voltage and/or fail cause
- Enough screen real-estate on 13–15" panel to show all pad numbers directly on the die map; if not, colour-code by perimeter side (e.g. blue = north, red = south)

### 3. Camera

- Display a live camera feed at all times (no processing during idle)
- **Capture image frame** at `TEST_START` — test has been triggered
- If multiple captures per test cycle are needed in future, all are linked to the same test record
- Images are saved to disk; v1 does not attempt to read serial numbers or perform OCR from the image
- Image capture starts after TEST_START; allow ~100 ms after trigger for a clean frame (operator's hand should be clear by then)
- Camera parameters (exposure, brightness, contrast, white balance, gain) are adjustable via named presets and manual controls (see §5)
- **Focus sharpness indicator** displayed on the live camera feed (e.g. Laplacian variance score + green/yellow/red bar) so the operator can adjust focus while watching the preview; update at frame rate with minimal processing overhead

### 4. Result record and storage

Each completed test is appended as a single self-contained JSON line to a JSONL log file. No separate export step is needed — results are always persisted to disk as they arrive.

**JSONL format** — one JSON object per line, one line per test:

| Field | Source |
|-------|--------|
| `timestamp` | Host clock (ISO 8601, primary record key) |
| `operator` | Optional free-text entry, persisted until changed |
| `adapter_uid` | EEPROM via GET_ADAPTER |
| `adapter_model` / `adapter_version` | GET_ADAPTER |
| `padmap_id` / `padmap_name` | GET_ADAPTER |
| `adapter_insertion_count` | GET_ADAPTER |
| `outcome` | SUMMARY line — `"PASS"` or `"FAIL"` |
| `pads` | Array of per-pad objects: `{die_pad, bp, bond, senseV, prevV, nextV, prevSh, nextSh}` |
| `slots` | Array of slot objects: `{slot, present, tested}` |
| `image_path` | Relative path to captured image |

**File layout:**

```
data/
  logs/
    2026-06-02.jsonl      ← one file per calendar day, auto-created
    2026-06-03.jsonl
    …
  images/
    2026/06/02/
      20260602T104231_PASS.png
      20260602T104218_FAIL.png
      …
```

- Append-only: each test result is one line; mid-test crash loses at most the in-progress line
- No schema, no migrations — new fields are added to JSON objects without breaking old readers
- Inspectable with standard tools: `cat`, `grep`, `jq`, Python `json.loads(line)`
- A future history / analysis tool can build a SQLite index from JSONL on demand

### 5. Settings (engineer access)

- Serial port selection (override auto-detect)
- Camera device selection
- Camera parameters via named presets (exposure, brightness, contrast, white balance, gain)
- Manual override of any individual camera parameter
- Ability to save the current manual config as a new named preset
- Default preset ("Standard") shipped with the application
- Active preset persisted across sessions
- Image save path and log directory
- Pad map override (SET_PADMAP)

---

## Non-Functional Requirements

| Requirement | Target |
|-------------|--------|
| OS | Linux (Xubuntu 24.04 LTS) |
| Framework | PySide6 (Python widgets + QPainter for die view) |
| UI feel | Touch-optimised, uncluttered — designed for 7–10" panel, 1024×600 minimum |
| Result display latency | < 500 ms from SUMMARY received to die map rendered |
| Camera frame rate | ≥ 15 fps live preview |
| Image capture latency | < 200 ms from trigger event to frame saved |
| Data storage | JSONL log files (append-only) + image files on disk |
| Packaging | Single AppImage or `.deb` — no separate Python install required |

---

## UI Layout (sketch)

```
┌─────────────────────────────────────────────────────────────┐
│  BondStation          ● Connected  Mezzanine70 v2  [⚙]      │
├───────────────────────┬─────────────────────────────────────┤
│   CAMERA FEED         │   DIE MAP                           │
│   (live, always on)   │   (pad ring, coloured by result,    │
│   ▓▓▓▓▓▓░░ FOCUS     │    all pad numbers visible)         │
│                       │                                     │
│                       │                                     │
├───────────────────────┴─────────────────────────────────────┤
│  Label: [___________]        [  TRIGGER TEST  ]    ✓ PASS   │
└─────────────────────────────────────────────────────────────┘
```

---

## Constraints

- **Single DUT slot only.** BondStation assumes a single-slot adapter. The firmware supports up to 5 slots, but the factory line workflow is one die at a time — multi-slot adapters are not handled by the GUI.
- **No offline mode.** BondStation requires a connected tester to function. It does not operate independently.
- **USB disconnect mid-test.** If the serial connection drops during a test (no SUMMARY received), BondStation must show an error state ("test interrupted, result unknown") and not hang waiting for data. Reconnect is handled by §1 auto-reconnect, but the interrupted test is logged as incomplete.

---

## Do3Think Camera Integration Spike

The Do3Think camera uses a proprietary SDK rather than standard v4l2. This has been validated on **Xubuntu 24.04 LTS** with the 2022 SDK and Python 3.13:

1. ~~Confirm the SDK compiles and runs on the target Linux platform~~ ✅ Confirmed on Xubuntu 24.04 LTS
2. ~~Write a minimal Python wrapper (ctypes or pybind11) to grab a single frame~~ ✅ Working
3. ~~Validate frame format conversion to OpenCV (BGR numpy array)~~ ✅ Working
4. ~~Measure: can we sustain ≥ 15 fps with the SDK overhead?~~ ✅ Sustained using PySide6 live view

Camera preset parameter names must map to the Do3Think SDK API (or v4l2 controls if falling back), so the preset layer must be hardware-agnostic.

**If the SDK becomes unviable in future**, fall back to any v4l2-compatible USB camera for v1.

---

## Out of Scope (v1)

- Die ID / serial number reading from camera image (v1 uses timestamp as record key)
- Die alignment / positioning from camera image
- Multiple simultaneous testers
- Network / cloud result storage
- Report generation (PDF etc.)
- User authentication / access control
- Pass/fail external signal (light stack, conveyor) — tester LEDs handle this
- Result history / session browser — to be addressed as a separate tool
- CSV export of result logs — JSONL files are directly usable by engineers; a dedicated export UI is not needed

---

## Deferred (post-v1)

- **Die ID / serial reading from camera image** — once the serial format on the PCB is defined and validated (printed text, QR, laser engraving), add OCR/barcode decoding triggered at test start
- **Wirebond defect detection** — CV pipeline to analyse captured images for visual defects
- **Wafer run ID workflow** — batch tracking keyed by operator-entered run label
- **Multi-slot DUT support** — UI for adapters with >1 DUT slot