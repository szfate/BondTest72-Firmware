import pytest
from PySide6.QtCore import QRectF

from bondtest72_gui.die_shapes import SHAPES_BY_PADMAP, SHAPE_1X1
from bondtest72_gui.die_view import fit_die, place_pads

AREA = QRectF(0, 0, 600, 500)


@pytest.mark.parametrize("shape", sorted(set(SHAPES_BY_PADMAP.values()), key=lambda s: s.name),
                         ids=lambda s: s.name)
def test_every_pad_is_placed_inside_the_die_without_overlap(shape):
    die = fit_die(shape, AREA)
    placements = place_pads(shape, die)

    assert len(placements) == len(shape.ring)
    assert die.width() / die.height() == pytest.approx(shape.aspect_ratio)
    for a in placements:
        assert die.contains(a.rect)
    for i, a in enumerate(placements):
        for b in placements[i + 1:]:
            assert not a.rect.intersects(b.rect), (a.pad.die_pad, b.pad.die_pad)


def test_ring_runs_counter_clockwise_from_the_top_right():
    die = fit_die(SHAPE_1X1, AREA)
    by_pad = {p.pad.die_pad: p for p in place_pads(SHAPE_1X1, die)}
    first, last_top, first_left = by_pad[0], by_pad[16], by_pad[17]

    assert first.side == "top" and first.rect.center().x() > last_top.rect.center().x()
    assert first_left.side == "left" and first_left.rect.center().y() < by_pad[18].rect.center().y()
    assert by_pad[37].side == "bottom" and by_pad[37].rect.center().x() < by_pad[38].rect.center().x()
    assert by_pad[73].side == "right" and by_pad[73].rect.center().y() < by_pad[72].rect.center().y()


def test_too_small_area_gives_an_empty_die():
    assert fit_die(SHAPE_1X1, QRectF(0, 0, 30, 30)).isEmpty()


def test_legend_names_each_fill_and_outline_colour():
    import os

    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    from PySide6.QtWidgets import QApplication

    from bondtest72_gui.die_view import DieLegend

    QApplication.instance() or QApplication([])
    text = DieLegend().text()
    for label in ("Good", "Open", "Not tested yet", "Not measured", "Supply", "Ground"):
        assert f"&nbsp;{label}" in text
