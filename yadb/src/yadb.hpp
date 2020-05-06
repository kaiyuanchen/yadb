#pragma once

#include <atomic>
#include <assert.h>
#include <iostream>
#include <functional>
#include <memory>
#include <mutex>
#include <string.h>
using namespace std;
using namespace chrono;

#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_RED "\033[1;31m"
#define GREEN "\033[0;32;32m"
#define LIGHT_GREEN "\033[1;32m"
#define BLUE "\033[0;32;34m"
#define LIGHT_BLUE "\033[1;34m"
#define DARY_GRAY "\033[1;30m"
#define CYAN "\033[0;36m"
#define LIGHT_CYAN "\033[1;36m"
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN "\033[0;33m"
#define YELLOW "\033[1;33m"
#define LIGHT_GRAY "\033[0;37m"
#define WHITE "\033[1;37m"

#ifdef _DEBUG
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define DEBUG(format, args...) printf( RED "[%.8s:%d] " NONE format "\n", __FILENAME__, __LINE__, ##args)
#else
#define DEBUG(args...)
#endif

#define MAX_FD 1024* 8
#define READER_BUF_SIZE 8192

#define KV template<class K, class V>
#define ARG1 std::placeholders::_1
#define ARG2 std::placeholders::_2
#define ARG3 std::placeholders::_3
#define ARG4 std::placeholders::_4

#define NOW std::chrono::system_clock::now()
#define DURATION_MICOR(last) std::chrono::duration_cast<std::chrono::microseconds>(NOW - last);
#define DURATION_MILLI(last) std::chrono::duration_cast<std::chrono::milliseconds>(NOW - last);

#define SHARED_FROM_BASE \
	template <typename derived>\
	std::shared_ptr<derived> shared_from_base(){\
		return std::static_pointer_cast<derived>(shared_from_this());\
	}

enum class RC{
	OK = 0,
	ERROR
};

typedef string bytes;
typedef function<RC(bytes, bytes)> dispatcher;

struct disk_loc {
	int file_id = 0;
	int rec_id = 0;
	int rec_size = 0;
};