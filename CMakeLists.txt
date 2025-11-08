cmake_minimum_required(VERSION 3.20)
project(atirgul)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCE_FILES "source/*.c")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(PANGO REQUIRED pango)
pkg_check_modules(PANGOCAIRO REQUIRED pangocairo)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GLIB2_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${PANGO_INCLUDE_DIRS}
    ${PANGOCAIRO_INCLUDE_DIRS}
)

# Ensure the pkg-config library dirs are available for the linker
target_link_directories(${PROJECT_NAME} PRIVATE
    ${GLIB_LIBRARY_DIRS}
    ${CAIRO_LIBRARY_DIRS}
    ${PANGO_LIBRARY_DIRS}
    ${PANGOCAIRO_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GLIB2_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PANGO_LIBRARIES}
    ${PANGOCAIRO_LIBRARIES}
)

add_subdirectory(example)
