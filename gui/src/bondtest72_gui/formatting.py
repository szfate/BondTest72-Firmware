"""Human-readable text for measurements, shared by the die view and pad table."""

from __future__ import annotations

from .models import PadResult

# The firmware reports 1e9 Ω for a reading that didn't conduct at all; anything
# this high is shown as open rather than as a number.
OPEN_OHMS = 1e8


def format_ohms(ohms: float) -> str:
    if ohms >= OPEN_OHMS:
        return "open"
    for scale, suffix in ((1e6, "M"), (1e3, "k")):
        if ohms >= scale:
            return f"{ohms / scale:.3g}{suffix}"
    return f"{ohms:.3g}"


def format_volts(volts: float) -> str:
    return f"{volts:.3f}"


def direction_text(pad: PadResult, forward: bool) -> str:
    """One direction's voltages: the Kelvin voltage per drive current for STD
    pads, the charging curve for CAP pads, or an empty string if that
    direction wasn't measured."""
    volts = pad.volts(forward)
    if not volts:
        return ""
    return (" " if pad.is_cap else " / ").join(format_volts(v) for v in volts) + " V"


def resistance_text(pad: PadResult, forward: bool) -> str:
    """One direction's apparent bond resistances (STD pads only), or an empty string."""
    readings = pad.rf if forward else pad.rr
    return " / ".join(format_ohms(r) for r in readings) + " Ω" if readings else ""


def pad_summary(pad: PadResult) -> str:
    """Multi-line description of one pad, e.g. for a tooltip."""
    lines = [f"Die pad {pad.dp} · adapter pin {pad.apin}",
             f"{pad.role.value} · {pad.method} · {pad.result.value}"]
    for label, forward in (("Reverse", False), ("Forward", True)):
        text = direction_text(pad, forward)
        if text:
            lines.append(f"{label}: {text}")
        ohms = resistance_text(pad, forward)
        if ohms:
            lines.append(f"    {ohms}")
    return "\n".join(lines)


def format_current(ua: float) -> str:
    return f"{ua:g} µA"


def format_time(us: float) -> str:
    return f"{us / 1000:g} ms" if us >= 1000 else f"{us:g} µs"
