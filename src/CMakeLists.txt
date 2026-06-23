# Feather PDF — application target.
# Copyright (C) 2026 Feather PDF contributors. Licensed under GPLv3 (see LICENSE).

qt_add_executable(feather-pdf
    app/main.cpp
    app/MainWindow.cpp
    app/MainWindow.h
    core/FeatherDocument.cpp
    core/FeatherDocument.h
    ui/Theme.cpp
    ui/Theme.h
    ui/Viewport.cpp
    ui/Viewport.h
    ui/PageView.cpp
    ui/PageView.h
    ui/HomeView.cpp
    ui/HomeView.h
    ui/ThumbnailPanel.cpp
    ui/ThumbnailPanel.h
    ui/OutlinePanel.cpp
    ui/OutlinePanel.h
    ui/TabStrip.cpp
    ui/TabStrip.h
    ui/CommandBar.cpp
    ui/CommandBar.h
    ui/FindBar.cpp
    ui/FindBar.h
    ui/NavigationRail.cpp
    ui/NavigationRail.h
    ui/ToolsPane.cpp
    ui/ToolsPane.h
    ui/FloatingPill.cpp
    ui/FloatingPill.h
    ui/Toast.cpp
    ui/Toast.h
    # Embedded symbolic icons (lucide SVGs → ":/icons/<name>.svg"); AUTORCC compiles it.
    ${CMAKE_SOURCE_DIR}/resources/feather.qrc
)

# Allow includes like "core/FeatherDocument.h" from anywhere in the tree.
target_include_directories(feather-pdf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(feather-pdf PRIVATE
    FEATHERPDF_VERSION="${PROJECT_VERSION}"
    FEATHERPDF_APP_ID="io.github.s4rt4.FeatherPDF"
)

target_link_libraries(feather-pdf PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Pdf
    Qt6::PdfWidgets
    Qt6::Svg
    Qt6::Concurrent
)

install(TARGETS feather-pdf RUNTIME DESTINATION bin)

# Desktop integration — the .desktop file (app-id io.github.s4rt4.FeatherPDF) and
# the brand logo installed into the hicolor icon theme under that same id, so the
# GNOME/Wayland shell shows the logo in the dash/dock and as the window icon.
install(FILES ${CMAKE_SOURCE_DIR}/packaging/io.github.s4rt4.FeatherPDF.desktop
    DESTINATION share/applications)
install(FILES ${CMAKE_SOURCE_DIR}/resources/icons/feather-logo.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME io.github.s4rt4.FeatherPDF.svg)
