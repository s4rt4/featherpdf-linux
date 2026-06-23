# Feather PDF — light on the system, full-featured on PDF.
# Copyright (C) 2026 Feather PDF contributors. Licensed under GPLv3 (see LICENSE).

cmake_minimum_required(VERSION 3.21)

project(FeatherPDF
    VERSION 0.0.1
    DESCRIPTION "Native, open-source PDF tool for Linux"
    HOMEPAGE_URL "https://github.com/s4rt4/featherpdf-linux"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a sensible build type for single-config generators.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# M0 needs: Core/Gui/Widgets (shell), Pdf/PdfWidgets (PDFium rendering),
# Svg (symbolic icons), Concurrent (off-UI-thread work). Poppler/QPDF are added
# when their modules land.
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Pdf PdfWidgets Svg Concurrent)

# Tests are built by default; turn off with -DBUILD_TESTING=OFF.
option(BUILD_TESTING "Build the test suite" ON)

# QPDF (lossless structure writer) ships no CMake config, so use pkg-config.
find_package(PkgConfig REQUIRED)
pkg_check_modules(QPDF REQUIRED IMPORTED_TARGET libqpdf)

qt_standard_project_setup()

# qt_standard_project_setup() turns on AUTOMOC/AUTOUIC but not AUTORCC; we embed
# the symbolic-icon resource via a .qrc, so enable it explicitly.
set(CMAKE_AUTORCC ON)

add_subdirectory(src)

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()
