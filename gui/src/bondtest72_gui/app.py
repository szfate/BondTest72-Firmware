"""Entry point: `bondtest72-gui` or `python -m bondtest72_gui`."""

from __future__ import annotations

import sys
from pathlib import Path

from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QApplication

from . import theme
from .main_window import MainWindow


ICON = Path(__file__).with_name("icon.svg")


def main() -> int:
    app = QApplication(sys.argv)
    # Same look on macOS, Windows and Linux, following the system's light/dark setting.
    theme.apply(app)
    app.setOrganizationName("BondTest72")
    app.setApplicationName("BondTest72")
    app.setWindowIcon(QIcon(str(ICON)))        # window and taskbar; the Dock on macOS
    window = MainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
