from .result_types import BondResult, PadRole, PadResult, SlotResult, TestResult
from .die_shapes import DieShape, PadInfo, shape_1x1
from .log_parser import parse_log


def __getattr__(name):
    if name == "DieMapWidget":
        from .die_map_widget import DieMapWidget
        return DieMapWidget
    if name == "build_results_table":
        from .die_map_widget import build_results_table
        return build_results_table
    if name == "create_main_layout":
        from .die_map_widget import create_main_layout
        return create_main_layout
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


__all__ = [
    "BondResult", "PadRole", "PadResult", "SlotResult", "TestResult",
    "DieShape", "PadInfo", "shape_1x1",
    "DieMapWidget", "build_results_table", "create_main_layout",
    "parse_log",
]