cmake_minimum_required(VERSION 3.10)
project(pdf2image VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)

# Poppler C++：Ubuntu 上有 poppler-cpp.pc，可用 pkg-config
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

# libhpdf：Ubuntu/Debian 的 libhpdf-dev 不提供 hpdf.pc，
# 因此用 find_path + find_library 手动探测
find_path(HPDF_INCLUDE_DIR hpdf.h)
find_library(HPDF_LIBRARY NAMES hpdf libhpdf)

if(NOT HPDF_INCLUDE_DIR OR NOT HPDF_LIBRARY)
    message(FATAL_ERROR "libhpdf not found. Install: sudo apt install libhpdf-dev")
endif()

add_library(HPDF::HPDF UNKNOWN IMPORTED)
set_target_properties(HPDF::HPDF PROPERTIES
    IMPORTED_LOCATION "${HPDF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${HPDF_INCLUDE_DIR}"
)

add_executable(pdf2image main.cpp)

target_link_libraries(pdf2image PRIVATE
    PkgConfig::POPPLER_CPP
    HPDF::HPDF
)

install(TARGETS pdf2image RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )
endif()
