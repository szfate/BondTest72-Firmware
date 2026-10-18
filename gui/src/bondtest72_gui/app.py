"""Entry point: `bondtest72-gui` or `python -m bondtest72_gui`."""

from __future__ import annotations

import sys

from PySide6.QtWidgets import QApplication

from .main_window import MainWindow


def main() -> int:
    app = QApplication(sys.argv)
    # Same look on macOS, Windows and Linux rather than each platform's native
    # style. Fusion still follows the system's light/dark setting.
    app.setStyle("Fusion")
    app.setOrganizationName("BondTest72")
    app.setApplicationName("BondTest72")
    window = MainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
