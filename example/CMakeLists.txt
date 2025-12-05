set(SAMPLE_BIN ${PROJECT_NAME}_sample)
# Build Objective-C++ Cocoa example
add_executable(${SAMPLE_BIN} main.mm)
target_include_directories(${SAMPLE_BIN} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${SAMPLE_BIN} PRIVATE ${PROJECT_NAME} "-framework Cocoa" "-framework CoreGraphics")
