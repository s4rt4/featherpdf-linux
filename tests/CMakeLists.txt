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

qt_add_executable(test_formeditor
    test_formeditor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/FormEditor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/FormEditor.h
)
target_include_directories(test_formeditor PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_formeditor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    PkgConfig::QPDF
)
add_test(NAME formeditor COMMAND test_formeditor)
set_tests_properties(formeditor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(test_annotator
    test_annotator.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/Annotator.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/Annotator.h
)
target_include_directories(test_annotator PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_annotator PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    PkgConfig::QPDF
)
add_test(NAME annotator COMMAND test_annotator)
set_tests_properties(annotator PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(test_formroundtrip
    test_formroundtrip.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/FormEditor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/FormEditor.h
    ${CMAKE_SOURCE_DIR}/src/backends/FormFiller.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/FormFiller.h
)
target_include_directories(test_formroundtrip PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_formroundtrip PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    PkgConfig::QPDF
    PkgConfig::POPPLER_QT6
)
add_test(NAME formroundtrip COMMAND test_formroundtrip)
set_tests_properties(formroundtrip PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(test_texteditor
    test_texteditor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/TextEditor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/TextEditor.h
    ${CMAKE_SOURCE_DIR}/src/backends/Annotator.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/Annotator.h
)
target_include_directories(test_texteditor PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_texteditor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    PkgConfig::QPDF
)
add_test(NAME texteditor COMMAND test_texteditor)
set_tests_properties(texteditor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(test_xfdf
    test_xfdf.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/Xfdf.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/Xfdf.h
    ${CMAKE_SOURCE_DIR}/src/backends/FormFiller.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/FormFiller.h
    ${CMAKE_SOURCE_DIR}/src/backends/FormEditor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/FormEditor.h
)
target_include_directories(test_xfdf PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_xfdf PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    PkgConfig::QPDF
    PkgConfig::POPPLER_QT6
)
add_test(NAME xfdf COMMAND test_xfdf)
set_tests_properties(xfdf PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(test_document
    test_document.cpp
    ${CMAKE_SOURCE_DIR}/src/core/FeatherDocument.cpp
    ${CMAKE_SOURCE_DIR}/src/core/FeatherDocument.h
)
target_include_directories(test_document PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_document PRIVATE Qt6::Core Qt6::Gui Qt6::Pdf Qt6::Test)
add_test(NAME document COMMAND test_document)
set_tests_properties(document PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(test_converter
    test_converter.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/Converter.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/Converter.h
)
target_include_directories(test_converter PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_converter PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    PkgConfig::POPPLER_QT6
)
add_test(NAME converter COMMAND test_converter)
set_tests_properties(converter PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(test_imageexporter
    test_imageexporter.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/ImageExporter.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/ImageExporter.h
)
target_include_directories(test_imageexporter PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_imageexporter PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Pdf
    Qt6::Test
)
add_test(NAME imageexporter COMMAND test_imageexporter)
set_tests_properties(imageexporter PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(test_patternredactor
    test_patternredactor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/PatternRedactor.cpp
    ${CMAKE_SOURCE_DIR}/src/backends/PatternRedactor.h
)
target_include_directories(test_patternredactor PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_patternredactor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    PkgConfig::POPPLER_QT6
)
add_test(NAME patternredactor COMMAND test_patternredactor)
set_tests_properties(patternredactor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
