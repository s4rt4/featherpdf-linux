// Feather PDF — light on the system, full-featured on PDF.
// Copyright (C) 2026 Feather PDF contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "app/MainWindow.h"

#include "ui/Theme.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Feather PDF"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("github.com/s4rt4"));
    QCoreApplication::setApplicationName(QStringLiteral("Feather PDF"));
    QCoreApplication::setApplicationVersion(QStringLiteral(FEATHERPDF_VERSION));
    // Lets the GNOME shell / Wayland associate the window with its .desktop file.
    QGuiApplication::setDesktopFileName(QStringLiteral(FEATHERPDF_APP_ID));

    // The design system: tokens + global style sheet, following the system
    // light/dark preference (ui-guidelines §2).
    Theme::instance().apply();

    MainWindow window;
    window.show();

    // Open the document AFTER the window is shown and the event loop is running.
    // Loading is synchronous (QtPdf parses + QPdfView lays out every page on the
    // UI thread); doing it before exec() leaves the process unresponsive to
    // Wayland before any window maps, which stalls the whole compositor on huge
    // documents. Queuing it lets the shell paint first, then go busy. The real
    // fix — off-thread render — lands with the custom render pipeline.
    if (argc > 1) {
        const QString path = QString::fromLocal8Bit(argv[1]);
        QTimer::singleShot(0, &window, [&window, path] { window.openPath(path); });
    }

    return app.exec();
}
