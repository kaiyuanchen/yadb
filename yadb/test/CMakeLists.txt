include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include(FindProtobuf)
find_package(Protobuf REQUIRED)

enable_testing()
FILE(GLOB _ALL_FILES ${CMAKE_CURRENT_SOURCE_DIR}//*_test.cpp)
FOREACH(_FILE ${_ALL_FILES})
	get_filename_component(OUT_NAME ${_FILE} NAME_WE)
	add_executable(${OUT_NAME} ${_FILE})
	target_link_libraries(${OUT_NAME} ${GTEST_BOTH_LIBRARIES})
	target_link_libraries(${OUT_NAME} ${CMAKE_THREAD_LIBS_INIT})
	target_link_libraries(
	    ${OUT_NAME} 
		thread_lib
		serv_lib
		proto_lib
		journal_lib
		storage_lib
	    index_lib
		parser_lib
		-lboost_system 
		-lboost_filesystem)
	target_link_libraries(${OUT_NAME} ${PROTOBUF_LIBRARY})
	#add_test(${OUT_NAME}m valgrind --show-possibly-lost=no --show-reachable=no ./${OUT_NAME}) 
	add_test(${OUT_NAME} ./${OUT_NAME})
ENDFOREACH()