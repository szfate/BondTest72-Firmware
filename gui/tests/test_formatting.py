from bondtest72_gui.formatting import direction_text, format_ohms, pad_summary, resistance_text
from bondtest72_gui.models import BondResult, PadResult, PadRole


def test_ohms_use_engineering_suffixes():
    assert [format_ohms(r) for r in (903.0, 5924.0, 48069.0, 2.2e6)] == ["903", "5.92k", "48.1k", "2.2M"]


def test_firmware_open_sentinel_reads_open():
    assert format_ohms(1e9) == "open"


def test_direction_text_shows_volts_for_resistance_and_charging_curve():
    std = PadResult(apin=63, dp=0, method="STD_REV", result=BondResult.GOOD,
                    rr=[48069.0, 5924.0, 903.0], vr=[0.484, 0.587, 0.878])
    cap = PadResult(apin=9, dp=17, method="CAP_REV", result=BondResult.GOOD, role=PadRole.VDDIO,
                    vrs=[0.1, 0.25])
    assert direction_text(std, forward=False) == "0.484 / 0.587 / 0.878 V"
    assert direction_text(std, forward=True) == ""          # forward not measured
    assert direction_text(cap, forward=False) == "0.100 0.250 V"
    assert resistance_text(std, forward=False) == "48.1k / 5.92k / 903 Ω"
    assert resistance_text(cap, forward=False) == ""


def test_pad_summary_lists_volts_then_ohms_for_measured_directions():
    pad = PadResult(apin=2, dp=13, method="STD_REV", result=BondResult.OPEN,
                    rr=[428653.0, 1e9, 1e9], vr=[1.996, 3.3, 3.3])
    assert pad_summary(pad) == ("Die pad 13 · adapter pin 2\n"
                                "IO · STD_REV · OPEN\n"
                                "Reverse: 1.996 / 3.300 / 3.300 V\n"
                                "    429k / open / open Ω")
