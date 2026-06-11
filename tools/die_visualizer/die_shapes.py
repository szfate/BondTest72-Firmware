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