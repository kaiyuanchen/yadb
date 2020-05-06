cmake_minimum_required(VERSION 2.8)
project(yadb)
include(FindProtobuf)

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(NONE 		  "${Esc}[m")
  set(RED         "${Esc}[31m")
  set(GREEN       "${Esc}[32m")
  set(YELLOW      "${Esc}[33m")
  set(BLUE        "${Esc}[34m")
  set(CYAN        "${Esc}[36m")
  set(WHITE       "${Esc}[37m")
endif()

find_package(GTest REQUIRED)

if(test)
set(CMAKE_CXX_FLAGS "-std=c++0x -pthread ${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
else()
set(CMAKE_CXX_FLAGS "-std=c++0x -O3 -pthread ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

include_directories(src)
add_subdirectory(src)

add_executable(${PROJECT_NAME} src/yadb.cpp)
target_link_libraries(
	${PROJECT_NAME} 
	serv_lib 
	proto_lib
	thread_lib
	parser_lib
	storage_lib
	journal_lib
	index_lib
	-lboost_thread 
	-lboost_system 
	-lboost_filesystem
	-lboost_program_options)
target_link_libraries(${PROJECT_NAME} ${PROTOBUF_LIBRARY})

add_executable(${PROJECT_NAME}-cli src/yadb-cli.cpp)
target_link_libraries(${PROJECT_NAME}-cli
	serv_lib 
	proto_lib
	thread_lib
	parser_lib
	storage_lib
	index_lib
	journal_lib
	-lboost_thread 
	-lboost_system 
	-lboost_filesystem
	-lboost_program_options)
target_link_libraries(${PROJECT_NAME}-cli ${PROTOBUF_LIBRARY})

if(test)
    include_directories(${GTEST_INCLUDE_DIRS})
	add_subdirectory(test)
	enable_testing()
endif()

#make clean
add_custom_target(rm COMMAND @rm *.idx *.index *.log *.journal ./test_sto/* 5566/* 7788/* 5678/* ||: )

#make run
add_custom_target(run
	COMMAND mkdir 5566 7788 5678
	COMMAND make rm
	COMMAND ./yadb --cluster=127.0.0.1:5566 &
	COMMAND ./yadb --cluster=127.0.0.1:7788 &
	COMMAND ./yadb --cluster=127.0.0.1:5678 &)

#make kill
#kill -9 $(ps aux | grep './yadb' | awk '{print $2}')
