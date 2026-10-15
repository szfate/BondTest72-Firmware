from bondtest72_gui import protocol as p


def test_hello_with_protocol_version():
    msg = p.parse_line("HELLO name=BondTest72 build=2026-10-15-0415793 uid=A1B2C3D4E5F60718 proto=2")
    assert msg == p.Hello(name="BondTest72", build="2026-10-15-0415793", uid="A1B2C3D4E5F60718", proto=2)


def test_hello_without_proto_is_version_1():
    msg = p.parse_line("HELLO name=BondTest72 build=2026-06-03-74fb8fd uid=A1B2C3D4E5F60718")
    assert isinstance(msg, p.Hello) and msg.proto == 1


def test_adapter_info():
    msg = p.parse_line(
        "ADAPTER aid=0123456789ABCDEF ahw=2 pm=2,5 active=2 lifespan=9999 mfg_date=20261015 "
        "ins=1 tests=4 eol=0 dut=1 state=FAIL")
    assert msg == p.AdapterInfo(aid="0123456789ABCDEF", ahw=2, pm=[2, 5], active=2, lifespan=9999,
                                mfg_date=20261015, ins=1, tests=4, eol=False, dut=True, state="FAIL")


def test_adapter_info_from_v1_firmware_has_no_active_or_state():
    msg = p.parse_line("ADAPTER aid=00 ahw=1 pm=2 lifespan=100 mfg_date=20260601 ins=3 tests=2 eol=0 dut=0")
    assert isinstance(msg, p.AdapterInfo)
    assert msg.active is None and msg.state is None


def test_padmap_entry_name_keeps_spaces():
    msg = p.parse_line("PADMAP id=2 pads=64 default=1 active=0 name=1x1 Mezzanine70 v2")
    assert msg == p.PadmapEntry(id=2, pads=64, default=True, active=False, name="1x1 Mezzanine70 v2")


def test_ok_with_and_without_id():
    assert p.parse_line("OK SET_PADMAP id=3") == p.Ok("SET_PADMAP", 3)
    assert p.parse_line("OK LIST_PADMAPS") == p.Ok("LIST_PADMAPS", None)


def test_error():
    assert p.parse_line("ERROR code=10 msg=PADMAP_INCOMPATIBLE") == p.Error(10, "PADMAP_INCOMPATIBLE")


def test_events():
    assert p.parse_line("EVENT DUT_INSERTED") == p.SimpleEvent("DUT_INSERTED")
    assert p.parse_line("EVENT WRONG_ORIENTATION") == p.SimpleEvent("WRONG_ORIENTATION")
    assert p.parse_line("EVENT FAULT msg=EEPROM_WRITE_FAILED") == p.Fault("EEPROM_WRITE_FAILED")
    assert p.parse_line("EVENT EOL_WARNING ins=101") == p.EolWarning(101)
    assert p.parse_line("EVENT ADAPTER_DETECTED aid=AB ahw=2 pm=2 active=2") == \
        p.AdapterDetected(aid="AB", ahw=2, pm=[2], active=2)


def test_test_start():
    msg = p.parse_line(
        "EVENT TEST_START aid=AB ahw=2 ins=1 tests=1 pm=2,5 active=2 current_list_ua=12,120,1325 "
        "max_bond_r_ohms=60000 cap_time_list_us=156,312,1250,10000,20000")
    assert isinstance(msg, p.TestStart)
    assert msg.active == 2 and msg.pm == [2, 5]
    assert msg.current_list_ua == [12.0, 120.0, 1325.0]
    assert msg.max_bond_r_ohms == 60000.0
    assert msg.cap_time_list_us == [156, 312, 1250, 10000, 20000]


def test_std_pad():
    msg = p.parse_line("PAD slot=0 apin=63 dp=0 type=IO method=STD_REV result=GOOD "
                       "rr=53579,6734,1097 vr=0.461,0.559,0.824")
    assert msg == p.Pad(slot=0, apin=63, dp=0, type="IO", method="STD_REV", result="GOOD",
                        rr=[53579.0, 6734.0, 1097.0], vr=[0.461, 0.559, 0.824])


def test_cap_pad():
    msg = p.parse_line("PAD slot=0 apin=9 dp=17 type=VDDIO method=CAP_REV result=OPEN "
                       "vrs=0.448,0.662,1.666,3.300,3.300")
    assert isinstance(msg, p.Pad)
    assert msg.type == "VDDIO" and msg.vrs == [0.448, 0.662, 1.666, 3.3, 3.3]
    assert msg.rr is None and msg.vfs is None


def test_pad_from_v1_firmware_has_no_type():
    msg = p.parse_line("PAD slot=0 apin=63 dp=0 method=STD_REV result=GOOD rr=1,2,3 vr=0.1,0.2,0.3")
    assert isinstance(msg, p.Pad) and msg.type is None


def test_slot_and_summary():
    assert p.parse_line("SLOT slot=0 present=1 tested=1") == p.Slot(0, True, True)
    assert p.parse_line("SUMMARY outcome=FAIL good=60 tested=64 fail_reason=DUT_REMOVED") == \
        p.Summary("FAIL", 60, 64, "DUT_REMOVED")
    assert p.parse_line("SUMMARY outcome=PASS good=64 tested=64").reason is None


def test_log_line():
    msg = p.parse_line("[  11264] [INFO] adapter: init ok, padmap=2 (1x1 Mezzanine70 v2)")
    assert msg == p.LogLine(11264, "INFO", "adapter: init ok, padmap=2 (1x1 Mezzanine70 v2)")


def test_unknown_and_blank_lines_are_ignored():
    for line in ["", "   ", "--- adapter self-test ---", "result: PASS", "SOMETHING_NEW a=1"]:
        assert p.parse_line(line) is None


def test_malformed_lines_return_none_instead_of_raising():
    assert p.parse_line("PAD slot=0 apin=63 method=STD_REV result=GOOD") is None   # no dp=
    assert p.parse_line("PAD slot=0 apin=63 dp=0 method=WEIRD result=GOOD") is None
    assert p.parse_line("PAD slot=0 apin=x dp=0 method=STD result=GOOD") is None
    assert p.parse_line("ERROR msg=NO_CODE") is None
    assert p.parse_line("EVENT TEST_START ahw=2 current_list_ua=1,a,3") is None


def test_unknown_fields_are_ignored():
    msg = p.parse_line("SLOT slot=0 present=1 tested=1 future_field=42")
    assert msg == p.Slot(0, True, True)


def test_commands():
    assert p.cmd_set_padmap(3) == "SET_PADMAP id=3\n"
    assert p.cmd_set_default_padmap(5) == "SET_DEFAULT_PADMAP id=5\n"
    assert p.cmd_list_padmaps() == "LIST_PADMAPS\n"
    assert p.cmd_run() == "RUN\n"
