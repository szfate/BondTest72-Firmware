import re

import pytest

from bondtest72_gui.die_shapes import SHAPES_BY_PADMAP, DieShape

from conftest import REPO_ROOT

REGISTRY = REPO_ROOT / "src" / "test" / "pad_map_registry.cpp"


def firmware_pad_maps() -> dict[int, list[tuple[int, str]]]:
    """(die pad, pad type) of every measured case, per pad map, read straight
    from the firmware's pad map tables. DISCHARGE steps are skipped — they are
    preparation, not measurements."""
    source = REGISTRY.read_text()
    maps = {}
    for block in re.finditer(r"_pm(\d+)Cases\[\] = \{(.*?)\n\};", source, re.S):
        cases = []
        body = re.sub(r"//[^\n]*", "", block.group(2))   # drop comments
        # IO shorthand macros: IO(apin, dp), IO3(...), IOM(...), ...
        for dp in re.findall(r"\bIO\w?\(\s*\d+\s*,\s*(\d+)\s*\)", body):
            cases.append((int(dp), "IO"))
        # Spelled-out cases: { ..., .diePad = 17, .strategy = ..., .padType = PadType::VDDIO, ... }
        for dp, strategy, ptype in re.findall(
                r"\.diePad\s*=\s*(\d+).*?\.strategy\s*=\s*TestStrategy::(\w+).*?\.padType\s*=\s*PadType::(\w+)",
                body, re.S):
            if strategy != "DISCHARGE":
                cases.append((int(dp), ptype))
        maps[int(block.group(1))] = cases
    return maps


def test_registry_parses():
    maps = firmware_pad_maps()
    assert sorted(maps) == [1, 2, 3, 4, 5, 6]
    assert len(maps[2]) == 64            # matches LIST_PADMAPS pads=64 for map 2


def test_every_firmware_pad_map_has_a_die_shape():
    assert set(firmware_pad_maps()) <= set(SHAPES_BY_PADMAP)


@pytest.mark.parametrize("padmap_id", sorted(SHAPES_BY_PADMAP))
def test_measured_pads_exist_on_the_die_with_matching_role(padmap_id):
    """The GUI draws results by die pad number, so every pad the firmware
    measures must exist on the die drawing — and be drawn with its real role."""
    shape = SHAPES_BY_PADMAP[padmap_id]
    for dp, ptype in firmware_pad_maps()[padmap_id]:
        role = shape.role_of(dp)
        assert role is not None, f"pad map {padmap_id}: dp{dp} is not on the {shape.name} die"
        assert role.value == ptype, f"pad map {padmap_id}: dp{dp} is {ptype} in firmware, {role.value} in die shape"


@pytest.mark.parametrize("shape", sorted(set(SHAPES_BY_PADMAP.values()), key=lambda s: s.name),
                         ids=lambda s: s.name)
def test_die_pad_numbers_are_unique(shape: DieShape):
    dps = [p.die_pad for p in shape.ring]
    assert len(dps) == len(set(dps))


def test_sides_split_the_ring_in_order():
    sides = SHAPES_BY_PADMAP[2].sides()
    assert [len(s) for s in sides.values()] == [17, 20, 17, 20]
    assert sides["top"][0].die_pad == 0 and sides["right"][-1].die_pad == 73


def test_side_counts_must_match_ring_length():
    with pytest.raises(ValueError):
        DieShape(name="bad", ring=(), top=1, left=0, bottom=0, right=0, aspect_ratio=1.0)
