import argparse
import sys

from die_visualizer.die_shapes import shape_1x1
from die_visualizer.die_map_widget import DieMapWidget, create_main_layout
from die_visualizer.log_parser import parse_log

from PySide6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget

PADMAP_SHAPES = {
    1: shape_1x1,
    2: shape_1x1,
}


def main():
    parser = argparse.ArgumentParser(description="BondTest72 die map visualizer")
    parser.add_argument("--file", required=True, help="Protocol log file to read")
    parser.add_argument("--padmap", type=int, default=2, help="Pad map ID (default: 2)")
    args = parser.parse_args()

    shape = PADMAP_SHAPES.get(args.padmap)
    if shape is None:
        print(f"Error: padmap {args.padmap} not yet supported")
        sys.exit(1)

    with open(args.file) as f:
        lines = f.readlines()

    result = parse_log(lines, shape=shape)
    if result is None:
        print("Error: no complete test result found in log")
        sys.exit(1)

    results_by_die_pad = {}
    for slot in result.slots:
        for p in slot.pads:
            results_by_die_pad[p.die_pad] = p

    app = QApplication(sys.argv)
    window = QMainWindow()
    window.setWindowTitle(f"BondTest72 — {shape.name} — {result.outcome} {result.good_count}/{result.tested_count}")

    central = QWidget()
    layout = QVBoxLayout(central)

    splitter = create_main_layout(shape, results_by_die_pad, result)
    layout.addWidget(splitter)

    window.setCentralWidget(central)
    window.resize(1200, 800)
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()