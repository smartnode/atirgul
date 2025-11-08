cmake_minimum_required(VERSION 3.20)
project(atirgul)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCE_FILES "source/*.c")

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CAIRO_INCLUDE_DIRS}
)

# Ensure the pkg-config library dirs are available for the linker
target_link_directories(${PROJECT_NAME} PRIVATE
    ${CAIRO_LIBRARY_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CAIRO_LIBRARIES}
)
