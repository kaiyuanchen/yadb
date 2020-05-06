FILE(GLOB _ALL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(thread_lib ${_ALL_FILES})