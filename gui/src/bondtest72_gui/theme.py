"""App look: qdarktheme's flat light/dark theme, following the system setting.

qdarktheme styles the standard widgets through its stylesheet and, by design,
sets only a partial palette. The die view and graphs paint themselves from the
palette, so the full theme palette is applied as well. The theme is re-applied
whenever the system switches between light and dark.
"""

from __future__ import annotations

import qdarktheme
from PySide6.QtCore import Qt
from PySide6.QtGui import QGuiApplication, QPalette
from PySide6.QtWidgets import QApplication


def _system_theme() -> str:
    scheme = QGuiApplication.styleHints().colorScheme()
    return "light" if scheme == Qt.ColorScheme.Light else "dark"


def apply(app: QApplication) -> None:
    """Apply the theme now and keep it in step with the system's light/dark setting."""
    app.setStyle("Fusion")          # base for anything the stylesheet leaves alone
    _apply(app)
    app.styleHints().colorSchemeChanged.connect(lambda _scheme: _apply(app))


def _apply(app: QApplication) -> None:
    theme = _system_theme()
    qdarktheme.setup_theme(theme)
    palette = qdarktheme.load_palette(theme)
    # The theme leaves AlternateBase white in dark mode; keep it a near-Base tone.
    palette.setColor(QPalette.ColorRole.AlternateBase,
                     palette.color(QPalette.ColorRole.Base).lighter(115) if theme == "dark"
                     else palette.color(QPalette.ColorRole.Base).darker(103))
    app.setPalette(palette)


def is_dark(palette: QPalette) -> bool:
    return palette.color(QPalette.ColorRole.Window).lightness() < 128
