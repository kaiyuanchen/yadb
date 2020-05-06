find_package(BISON)
find_package(FLEX)

BISON_TARGET(PARSER yaql.y ${CMAKE_CURRENT_BINARY_DIR}/yaql.tab.c "COMPILE_FLAGS" "-pyaql")
FLEX_TARGET(SCANNER yaql.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yaql.c "COMPILE_FLAGS" "-Pyaql")
ADD_FLEX_BISON_DEPENDENCY(SCANNER PARSER)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/yaql.tab.c PROPERTIES LANGUAGE CXX)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/lex.yaql.c PROPERTIES LANGUAGE CXX)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(parser_lib ${BISON_PARSER_OUTPUTS} ${FLEX_SCANNER_OUTPUTS})
#add_executable(main ${BISON_PARSER_OUTPUTS} ${FLEX_SCANNER_OUTPUTS} main.cc)


