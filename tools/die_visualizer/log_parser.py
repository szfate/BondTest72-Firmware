from .result_types import BondResult, PadRole, PadResult, SlotResult, TestResult


def _parse_kv(tokens: list[str]) -> dict:
    d = {}
    for t in tokens:
        if '=' in t:
            k, _, v = t.partition('=')
            d[k] = v
    return d


def _detect_format(line: str) -> str:
    parts = line.split()
    if len(parts) < 2:
        return 'positional'
    return 'kv' if '=' in parts[1] else 'positional'


def _parse_pad_kv(parts: list[str]) -> tuple[int, PadResult] | None:
    d = _parse_kv(parts[1:])
    if 'dp' not in d:
        return None
    bond_str = d.get('result', 'GOOD')
    if bond_str == 'SHORT':
        bond_str = 'SHORT_GND'
    slot = int(d.get('slot', '0'))
    pr = PadResult(
        apin=int(d.get('mez', d.get('apin', '0'))),
        die_pad=int(d['dp']),
        bond=BondResult(bond_str),
        prev_short=d.get('ps', '0') == '1',
        next_short=d.get('ns', '0') == '1',
        sense_v=float(d.get('sv', '0')),
        prev_v=float(d.get('pv', '0')),
        next_v=float(d.get('nv', '0')),
    )
    return (slot, pr)


def _parse_pad_positional(parts: list[str]) -> tuple[int, PadResult] | None:
    if len(parts) < 10:
        return None
    bond_str = parts[4]
    if bond_str == 'SHORT':
        bond_str = 'SHORT_GND'
    slot = int(parts[1])
    pr = PadResult(
        apin=int(parts[2]),
        die_pad=int(parts[3]),
        bond=BondResult(bond_str),
        prev_short=parts[5] == '1',
        next_short=parts[6] == '1',
        sense_v=float(parts[7]),
        prev_v=float(parts[8]),
        next_v=float(parts[9]),
    )
    return (slot, pr)


def _parse_slot_kv(parts: list[str]) -> SlotResult | None:
    d = _parse_kv(parts[1:])
    if 'slot' not in d:
        return None
    return SlotResult(
        slot=int(d['slot']),
        present=d.get('present', '0') == '1',
        tested=d.get('tested', '0') == '1',
    )


def _parse_slot_positional(parts: list[str]) -> SlotResult | None:
    if len(parts) < 4:
        return None
    return SlotResult(
        slot=int(parts[1]),
        present=parts[2] == '1',
        tested=parts[3] == '1',
    )


def _parse_summary_kv(parts: list[str]) -> tuple | None:
    d = _parse_kv(parts[1:])
    outcome = d.get('outcome')
    if outcome is None:
        return None
    good = int(d.get('good', '0'))
    tested = int(d.get('tested', '0'))
    return (outcome, good, tested)


def _parse_summary_positional(parts: list[str]) -> tuple | None:
    if len(parts) < 3:
        return None
    outcome = parts[1]
    counts = parts[2].split('/')
    if len(counts) != 2:
        return None
    return (outcome, int(counts[0]), int(counts[1]))


def parse_log(lines: list[str], shape=None) -> TestResult | None:
    """Parse a protocol log (list of lines) and return the latest TestResult.

    Supports both KV format (new) and positional format (old) with auto-detection.
    If `shape` is provided (a DieShape), each PadResult gets its role field
    populated from the shape's pad metadata.
    """
    pads_by_slot = {}
    slots = []
    outcome = None
    good_count = 0
    tested_count = 0

    for line in lines:
        line = line.strip()
        if not line:
            continue

        if line.startswith("PAD "):
            fmt = _detect_format(line)
            parts = line.split()
            parsed = _parse_pad_kv(parts) if fmt == 'kv' else _parse_pad_positional(parts)
            if parsed:
                slot_num, pr = parsed
                if shape:
                    info = shape.pad_by_die_pad(pr.die_pad)
                    if info:
                        pr.role = info.role
                pads_by_slot.setdefault(slot_num, []).append(pr)

        elif line.startswith("SLOT "):
            fmt = _detect_format(line)
            parts = line.split()
            sr = _parse_slot_kv(parts) if fmt == 'kv' else _parse_slot_positional(parts)
            if sr:
                slots.append(sr)

        elif line.startswith("SUMMARY "):
            fmt = _detect_format(line)
            parts = line.split()
            result = _parse_summary_kv(parts) if fmt == 'kv' else _parse_summary_positional(parts)
            if result:
                outcome, good_count, tested_count = result

    if outcome is None:
        return None

    if slots:
        for sr in slots:
            sr.pads = pads_by_slot.get(sr.slot, [])
    else:
        slot0_pads = pads_by_slot.get(0, [])
        slots = [SlotResult(slot=0, present=True, tested=True, pads=slot0_pads)]

    return TestResult(
        outcome=outcome,
        good_count=good_count,
        tested_count=tested_count,
        slots=slots,
    )