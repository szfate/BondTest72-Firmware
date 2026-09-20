from dataclasses import dataclass

from .result_types import PadRole


@dataclass
class PadInfo:
    die_pad: int
    role: PadRole


@dataclass
class DieShape:
    name: str
    padmap_id: int
    ring: list
    north_count: int
    east_count: int
    south_count: int
    west_count: int
    aspect_ratio: float = 0.85

    @property
    def total_pads(self) -> int:
        return len(self.ring)

    def pad_by_die_pad(self, die_pad: int):
        for p in self.ring:
            if p.die_pad == die_pad:
                return p
        raise ValueError(f"die_pad {die_pad} not found in ring")


def _io(dp: int) -> PadInfo:
    return PadInfo(die_pad=dp, role=PadRole.IO)


def _gnd(dp: int) -> PadInfo:
    return PadInfo(die_pad=dp, role=PadRole.GND)


def _vdd(dp: int) -> PadInfo:
    return PadInfo(die_pad=dp, role=PadRole.VDDIO)


def _vdd_core(dp: int) -> PadInfo:
    return PadInfo(die_pad=dp, role=PadRole.VDD_CORE)


def _pwr(dp: int) -> PadInfo:
    return PadInfo(die_pad=dp, role=PadRole.PWR_AUX)


def _nc(dp: int) -> PadInfo:
    return PadInfo(die_pad=dp, role=PadRole.NC)


def _bus(dp: int) -> PadInfo:
    return PadInfo(die_pad=dp, role=PadRole.GND)


# aspect_ratio = die width / die height.
# Width is driven by north/south sides (pads arranged horizontally).
# Height is driven by east/west sides  (pads arranged vertically).
# Approximate formula: max(north_count, south_count) / max(east_count, west_count).
#   > 1.0  → landscape (wider than tall)
#   < 1.0  → portrait  (taller than wide)
#
# Source: docs/DUT_PADMAP_1X1.md cross-reference table
# 74 die pads in physical ring order (DP 0–73, counter-clockwise from top-right)
# Die is portrait: short sides (N/S) have 17 pads, long sides (E/W) have 20 pads
shape_1x1 = DieShape(
    name="1x1 Mezzanine70",
    padmap_id=2,
    north_count=17,
    east_count=20,
    south_count=17,
    west_count=20,
    aspect_ratio=0.85,
    ring=[
        _io(0), _io(1), _io(2), _io(3), _io(4), _io(5), _io(6), _io(7),
        _gnd(8), _io(9), _io(10), _io(11), _io(12), _io(13), _io(14), _io(15), _io(16),
        _vdd(17), _gnd(18), _io(19), _io(20), _io(21), _io(22), _io(23), _io(24),
        _pwr(25), _gnd(26), _io(27), _io(28), _io(29), _io(30), _io(31), _io(32), _gnd(33),
        _vdd_core(34), _gnd(35), _vdd(36),
        _io(37), _io(38), _io(39), _io(40), _io(41), _io(42), _io(43), _io(44), _gnd(45),
        _io(46), _io(47), _io(48), _io(49), _io(50), _io(51), _io(52), _io(53),
        _io(54), _io(55), _gnd(56), _vdd(57), _io(58), _io(59), _io(60), _io(61),
        _gnd(62), _pwr(63), _io(64), _io(65), _io(66), _io(67), _io(68), _io(69),
        _gnd(70), _vdd_core(71), _gnd(72), _vdd(73),
    ],
)

# Source: docs/DUT_PADMAP_1X0P5.md cross-reference table
# 72 die pads in physical ring order (DP 0–71, counter-clockwise from top-right)
# Die is portrait: long sides (N/S) have 24 pads, short sides (E/W) have 12 pads
shape_1x0p5 = DieShape(
    name="1x0p5 Mezzanine70",
    padmap_id=3,
    north_count=24,
    east_count=12,
    south_count=24,
    west_count=12,
    aspect_ratio=2.0,
    ring=[
        # North (DP 0–23)
        _io(0), _io(1), _io(2), _io(3),
        _gnd(4), _vdd_core(5), _io(6), _io(7), _io(8), _io(9),
        _io(10), _io(11), _io(12), _io(13), _io(14), _io(15),
        _io(16), _io(17),
        _gnd(18), _vdd(19), _io(20), _io(21), _io(22), _io(23),
        # East (DP 24–35)
        _gnd(24), _pwr(25), _io(26), _io(27), _io(28), _io(29),
        _io(30), _io(31), _io(32), _io(33),
        _bus(34), _vdd(35),
        # South (DP 36–59)
        _io(36), _io(37), _io(38), _io(39),
        _vdd_core(40), _gnd(41), _io(42), _io(43), _io(44), _io(45),
        _io(46), _io(47), _io(48), _io(49), _io(50), _io(51),
        _io(52), _io(53), _vdd(54), _gnd(55), _io(56), _io(57), _io(58), _io(59),
        # West (DP 60–71)
        _pwr(60), _gnd(61), _io(62), _io(63), _io(64), _io(65),
        _io(66), _io(67), _io(68), _io(69), _vdd(70), _bus(71),
    ],
)

# Source: docs/DUT_PADMAP_0P5X1.md cross-reference table
# 72 die pads in physical ring order (DP 0–71, counter-clockwise from top-right)
# Die is 1.94 mm (N/S width) × 5.12 mm (E/W height): short sides (N/S) have
# 8 pads each, long sides (E/W) have 28 pads each (corrected 2026-09-15 —
# was 12/24/12/24 from the un-rotated 1x0p5 assumption)
# All 8 VCC pads are labelled VDDIO pending die rail documentation (see doc notes)
shape_0p5x1 = DieShape(
    name="0p5x1 Mezzanine70",
    padmap_id=5,
    north_count=8,
    east_count=28,
    south_count=8,
    west_count=28,
    aspect_ratio=1.94 / 5.12,
    ring=[
        # North (DP 0–7)
        _io(0), _io(1), _io(2),
        _bus(3), _vdd(4),
        _io(5), _io(6), _io(7),
        # East (DP 8–35)
        _io(8), _io(9),
        _gnd(10), _vdd(11),
        _io(12), _io(13), _io(14), _io(15), _io(16), _io(17), _io(18), _io(19),
        _io(20),
        _gnd(21), _vdd(22),
        _io(23), _io(24), _io(25), _io(26), _io(27), _io(28), _io(29), _io(30),
        _io(31),
        _gnd(32), _vdd(33),
        _io(34), _io(35),
        # South (DP 36–43)
        _io(36), _io(37), _io(38),
        _vdd(39), _bus(40),
        _io(41), _io(42), _io(43),
        # West (DP 44–71)
        _io(44), _io(45),
        _vdd(46), _gnd(47),
        _io(48), _io(49), _io(50), _io(51), _io(52), _io(53), _io(54), _io(55),
        _io(56),
        _vdd(57), _gnd(58), _io(59),
        _io(60), _io(61), _io(62), _io(63), _io(64), _io(65), _io(66),
        _vdd(67), _gnd(68),
        _io(69), _io(70), _io(71),
    ],
)

# Source: docs/DUT_PADMAP_TQVA.md cross-reference table
# Die is 1936 µm wide (N/S) × 2531 µm tall (E/W): N/S sides have 11 pads each,
# E/W sides have 18 pads each (die dimensions confirmed 2026-09). 11+18+11+18
# = 58 slots; 56 known die pads + 2 unaccounted (see below) close the ring.
# Die pad ring numbering for the two GND dut pins 61 (apin 10) and 53 (apin 18)
# was not in the source — placeholders dp 56/57 are appended to the west tail
# and are never referenced by test results (the firmware only uses die pads 0–50).
shape_tqva = DieShape(
    name="TQVA",
    padmap_id=6,
    north_count=11,
    east_count=18,
    south_count=11,
    west_count=18,
    aspect_ratio=1936 / 2531,
    ring=[
        # North (DP 0–10)
        _io(0), _io(1), _gnd(2), _vdd(3),
        _io(4), _io(5), _io(6), _io(7), _io(8), _io(9), _io(10),
        # East (DP 11–28)
        _io(11), _io(12), _io(13), _io(14), _io(15), _io(16), _io(17),
        _io(18), _io(19), _io(20), _io(21), _io(22), _io(23),
        _io(24), _io(25), _gnd(26), _vdd(27), _io(28),
        # South (DP 29–39)
        _io(29), _io(30), _io(31), _io(32), _nc(33), _nc(34), _vdd(35),
        _gnd(36), _nc(37), _nc(38), _io(39),
        # West (DP 40–57)
        _io(40), _vdd_core(41), _gnd(42),
        _nc(43), _nc(44),
        _io(45), _io(46), _io(47), _io(48), _io(49), _io(50),
        _nc(51), _nc(52), _nc(53), _nc(54), _nc(55),
        # dp 56/57: placeholder ring slots for the two GND dut pins 61/53 whose
        # die pads were not in the source (never referenced by test results)
        _gnd(56), _gnd(57),
    ],
)