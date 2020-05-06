#pragma once

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include "yadb.hpp"
using namespace std;

#define BPT_CACHE_SIZE 100
#define BPT_BLOCK_SIZE 100

struct bpt_header {
	uint32_t root_ptr = 0;
	uint32_t index_allocated = 0;
	uint32_t leaf_allocated = 0;
};

class bpt_fs{
public:
	bpt_fs();
	~bpt_fs();
	RC open(const char* path_);
	RC seek(int node_id);
	RC read(int32_t node_id, void* block, int32_t block_size);
	RC write(int32_t node_id, void* block, int32_t block_size);
	RC read_header(void* block, size_t block_size);
	RC write_header(void* block, size_t block_size);
	size_t get_size();
private:
	int fd= -1;
	//char* cache;
	//int* cache_ids;
};