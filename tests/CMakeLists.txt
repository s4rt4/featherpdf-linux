# Feather PDF — tests.
# Copyright (C) 2026 Feather PDF contributors. Licensed under GPLv3 (see LICENSE).

qt_add_executable(test_pdfeditor
    test_pdfeditor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/PdfEditor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/PdfEditor.h
)

target_include_directories(test_pdfeditor PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Linking Qt6::Gui (not Widgets) makes QTEST_MAIN use QGuiApplication, which runs
# headless under the offscreen platform plugin (set in the test environment).
target_link_libraries(test_pdfeditor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    PkgConfig::QPDF
)

add_test(NAME pdfeditor COMMAND test_pdfeditor)
set_tests_properties(pdfeditor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
