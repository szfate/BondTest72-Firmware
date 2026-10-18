"""Die outlines and pad rings, used to draw results on a picture of the die.

Each DieShape lists the die's pads in physical ring order, starting at the
top-right corner and running counter-clockwise:

    top    — right to left
    left   — top to bottom
    bottom — left to right
    right  — bottom to top

The docs/DUT_PADMAP_*.md files call these sides North, East, South and West
(in that ring order), so "East" is the side drawn on the left.

The tester reports results by die pad number (dp=), so drawing a result only
needs the ring position of each dp. Pad roles here are used for pads the
tester doesn't measure (GND, NC) and as a fallback for firmware that doesn't
send type= on PAD lines.
"""

from __future__ import annotations

from dataclasses import dataclass

from .models import PadRole


@dataclass(frozen=True)
class PadInfo:
    die_pad: int
    role: PadRole


@dataclass(frozen=True)
class DieShape:
    name: str
    ring: tuple[PadInfo, ...]   # all pads, in ring order (see module docstring)
    top: int                    # pad counts per side; they must add up to len(ring)
    left: int
    bottom: int
    right: int
    aspect_ratio: float         # die width / height, for the outline (< 1 = portrait)

    def __post_init__(self) -> None:
        if self.top + self.left + self.bottom + self.right != len(self.ring):
            raise ValueError(f"{self.name}: side counts don't add up to {len(self.ring)} pads")

    def sides(self) -> dict[str, tuple[PadInfo, ...]]:
        """Pads per side, each in ring order (see the module docstring for directions)."""
        a = self.top
        b = a + self.left
        c = b + self.bottom
        return {
            "top": self.ring[:a],
            "left": self.ring[a:b],
            "bottom": self.ring[b:c],
            "right": self.ring[c:],
        }

    def role_of(self, die_pad: int) -> PadRole | None:
        """Role of a die pad, or None if this die has no such pad."""
        for p in self.ring:
            if p.die_pad == die_pad:
                return p.role
        return None


# Shorthands that keep the ring tables below readable.
def _io(dp: int) -> PadInfo: return PadInfo(dp, PadRole.IO)
def _gnd(dp: int) -> PadInfo: return PadInfo(dp, PadRole.GND)
def _vdd(dp: int) -> PadInfo: return PadInfo(dp, PadRole.VDDIO)
def _core(dp: int) -> PadInfo: return PadInfo(dp, PadRole.VDD_CORE)
def _pwr(dp: int) -> PadInfo: return PadInfo(dp, PadRole.PWR_AUX)
def _bus(dp: int) -> PadInfo: return PadInfo(dp, PadRole.GND_BUS_ONLY)
def _nc(dp: int) -> PadInfo: return PadInfo(dp, PadRole.NC)


# 1x1 — docs/DUT_PADMAP_1X1.md. 74 pads (dp 0–73).
SHAPE_1X1 = DieShape(
    name="1x1",
    top=17, left=20, bottom=17, right=20,
    aspect_ratio=0.85,
    ring=(
        _io(0), _io(1), _io(2), _io(3), _io(4), _io(5), _io(6), _io(7),
        _gnd(8), _io(9), _io(10), _io(11), _io(12), _io(13), _io(14), _io(15), _io(16),
        _vdd(17), _gnd(18), _io(19), _io(20), _io(21), _io(22), _io(23), _io(24),
        _pwr(25), _gnd(26), _io(27), _io(28), _io(29), _io(30), _io(31), _io(32), _gnd(33),
        _core(34), _gnd(35), _vdd(36),
        _io(37), _io(38), _io(39), _io(40), _io(41), _io(42), _io(43), _io(44), _gnd(45),
        _io(46), _io(47), _io(48), _io(49), _io(50), _io(51), _io(52), _io(53),
        _io(54), _io(55), _gnd(56), _vdd(57), _io(58), _io(59), _io(60), _io(61),
        _gnd(62), _pwr(63), _io(64), _io(65), _io(66), _io(67), _io(68), _io(69),
        _gnd(70), _core(71), _gnd(72), _vdd(73),
    ),
)

# 1x0p5 — docs/DUT_PADMAP_1X0P5.md. 72 pads (dp 0–71), landscape.
SHAPE_1X0P5 = DieShape(
    name="1x0p5",
    top=24, left=12, bottom=24, right=12,
    aspect_ratio=2.0,
    ring=(
        # top (dp 0–23)
        _io(0), _io(1), _io(2), _io(3),
        _gnd(4), _core(5), _io(6), _io(7), _io(8), _io(9),
        _io(10), _io(11), _io(12), _io(13), _io(14), _io(15),
        _io(16), _io(17),
        _gnd(18), _vdd(19), _io(20), _io(21), _io(22), _io(23),
        # left (dp 24–35)
        _gnd(24), _pwr(25), _io(26), _io(27), _io(28), _io(29),
        _io(30), _io(31), _io(32), _io(33),
        _bus(34), _vdd(35),
        # bottom (dp 36–59)
        _io(36), _io(37), _io(38), _io(39),
        _core(40), _gnd(41), _io(42), _io(43), _io(44), _io(45),
        _io(46), _io(47), _io(48), _io(49), _io(50), _io(51),
        _io(52), _io(53), _vdd(54), _gnd(55), _io(56), _io(57), _io(58), _io(59),
        # right (dp 60–71)
        _pwr(60), _gnd(61), _io(62), _io(63), _io(64), _io(65),
        _io(66), _io(67), _io(68), _io(69), _vdd(70), _bus(71),
    ),
)

# MOSB — docs/DUT_PADMAP_MOSB.md. Same 74-pad ring geometry as 1x1, different
# pinout: one VDD (dp8; dp9 shares its adapter pin and is never tested → NC)
# and four GND pads, of which only dp54 has an adapter pin.
SHAPE_MOSB = DieShape(
    name="G801-MOSB",
    top=17, left=20, bottom=17, right=20,
    aspect_ratio=0.85,
    ring=(
        _io(0), _io(1), _io(2), _io(3), _io(4), _io(5), _io(6), _io(7),
        _vdd(8), _nc(9), _io(10), _io(11), _io(12), _io(13), _io(14), _io(15), _io(16),
        _gnd(17), _io(18), _io(19), _io(20), _io(21), _io(22), _io(23), _io(24),
        _io(25), _io(26), _io(27), _io(28), _io(29), _io(30), _io(31), _io(32),
        _io(33), _io(34), _io(35), _gnd(36),
        _io(37), _io(38), _io(39), _io(40), _io(41), _io(42), _io(43), _io(44),
        _io(45), _io(46), _io(47), _io(48), _io(49), _io(50), _io(51), _io(52),
        _io(53), _gnd(54),
        _io(55), _io(56), _io(57), _io(58), _io(59), _io(60), _io(61), _io(62),
        _io(63), _io(64), _io(65), _io(66), _io(67), _io(68), _io(69), _io(70),
        _io(71), _io(72), _gnd(73),
    ),
)

# 0p5x1 — docs/DUT_PADMAP_0P5X1.md. 72 pads (dp 0–71). Die is 1.94 × 5.12 mm;
# the long sides carry 28 pads each. All supply pads are labelled VDDIO until
# the die's rail documentation says otherwise (see the doc's notes).
SHAPE_0P5X1 = DieShape(
    name="0p5x1",
    top=8, left=28, bottom=8, right=28,
    aspect_ratio=1.94 / 5.12,
    ring=(
        # top (dp 0–7)
        _io(0), _io(1), _io(2),
        _bus(3), _vdd(4),
        _io(5), _io(6), _io(7),
        # left (dp 8–35)
        _io(8), _io(9),
        _gnd(10), _vdd(11),
        _io(12), _io(13), _io(14), _io(15), _io(16), _io(17), _io(18), _io(19),
        _io(20),
        _gnd(21), _vdd(22),
        _io(23), _io(24), _io(25), _io(26), _io(27), _io(28), _io(29), _io(30),
        _io(31),
        _gnd(32), _vdd(33),
        _io(34), _io(35),
        # bottom (dp 36–43)
        _io(36), _io(37), _io(38),
        _vdd(39), _bus(40),
        _io(41), _io(42), _io(43),
        # right (dp 44–71)
        _io(44), _io(45),
        _vdd(46), _gnd(47),
        _io(48), _io(49), _io(50), _io(51), _io(52), _io(53), _io(54), _io(55),
        _io(56),
        _vdd(57), _gnd(58), _io(59),
        _io(60), _io(61), _io(62), _io(63), _io(64), _io(65), _io(66),
        _vdd(67), _gnd(68),
        _io(69), _io(70), _io(71),
    ),
)

# TQVA — docs/DUT_PADMAP_TQVA.md. Die is 1936 × 2531 µm. 56 known die pads
# plus two placeholder slots (dp 56/57) for GND pins whose die pads aren't in
# the source; test results never reference them.
SHAPE_TQVA = DieShape(
    name="G801-TQVA",
    top=11, left=18, bottom=11, right=18,
    aspect_ratio=1936 / 2531,
    ring=(
        # top (dp 0–10)
        _io(0), _io(1), _gnd(2), _vdd(3),
        _io(4), _io(5), _io(6), _io(7), _io(8), _io(9), _io(10),
        # left (dp 11–28)
        _io(11), _io(12), _io(13), _io(14), _io(15), _io(16), _io(17),
        _io(18), _io(19), _io(20), _io(21), _io(22), _io(23),
        _io(24), _io(25), _gnd(26), _vdd(27), _io(28),
        # bottom (dp 29–39)
        _io(29), _io(30), _io(31), _io(32), _nc(33), _nc(34), _vdd(35),
        _gnd(36), _nc(37), _nc(38), _io(39),
        # right (dp 40–57)
        _io(40), _core(41), _gnd(42),
        _nc(43), _nc(44),
        _io(45), _io(46), _io(47), _io(48), _io(49), _io(50),
        _nc(51), _nc(52), _nc(53), _nc(54), _nc(55),
        _gnd(56), _gnd(57),
    ),
)

# Firmware pad map ID → die shape. Several pad maps can share a die: pad map 1
# is the early 1x1 board with one trace left unconnected.
SHAPES_BY_PADMAP: dict[int, DieShape] = {
    1: SHAPE_1X1,
    2: SHAPE_1X1,
    3: SHAPE_1X0P5,
    4: SHAPE_MOSB,
    5: SHAPE_0P5X1,
    6: SHAPE_TQVA,
}


def shape_for_padmap(padmap_id: int) -> DieShape | None:
    """The die drawing for a pad map, or None if this app has none for it yet."""
    return SHAPES_BY_PADMAP.get(padmap_id)
