# Feather PDF — application target.
# Copyright (C) 2026 Feather PDF contributors. Licensed under GPLv3 (see LICENSE).

qt_add_executable(feather-pdf
    app/main.cpp
    app/MainWindow.cpp
    app/MainWindow.h
    core/FeatherDocument.cpp
    core/FeatherDocument.h
    ui/Viewport.cpp
    ui/Viewport.h
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
    Qt6::Concurrent
)

install(TARGETS feather-pdf RUNTIME DESTINATION bin)
