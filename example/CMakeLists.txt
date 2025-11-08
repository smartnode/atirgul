set(SAMPLE_BIN ${PROJECT_NAME}_sample)
add_executable(${SAMPLE_BIN} main.cpp)
target_include_directories(${SAMPLE_BIN} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${SAMPLE_BIN} PRIVATE ${PROJECT_NAME})
