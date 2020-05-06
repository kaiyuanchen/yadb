#pragma once

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include "yadb.hpp"

using namespace std;
namespace boost_fs = boost::filesystem;

#define FILE_MAX_SIZE 1000
#define FILE_MAX_RECORD 100
#define FILE_BLOCK_SIZE 1000
#define FREE_BLOCK_OFF sizeof(file_meta)
#define REC_OFF sizeof(free_space)* FILE_MAX_RECORD + sizeof(file_meta)

struct file_meta{
	int allocate = 0;
	int free = 0;
};

class storage{
public:
	explicit storage(){};
	RC start(string path);  //open file, set free bit
	RC insert(bytes& rec, disk_loc& loc); //insert record
	RC read(disk_loc& loc, bytes& rec);
	RC write(shared_ptr<void> rec, int size);

	vector<int> fds;
	vector<shared_ptr<file_meta>> metas;
private:
	boost_fs::path current_path;
	RC create_file(boost_fs::path file_path);
	RC load_file(boost_fs::path file_path);
};

extern storage* store;