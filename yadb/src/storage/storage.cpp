#include "storage.hpp"
#include <fcntl.h>
#include <iostream>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
storage* store = nullptr;

RC storage::start(string path){
	if (boost_fs::exists(path) && !boost_fs::is_directory(path)) assert(false);
	current_path = path;
	boost_fs::path lock_file = "LOCK";
	boost_fs::path lock_path = current_path / lock_file;

	//lock storage
	int lock_fd = open(lock_path.string().c_str(), O_RDWR | O_CREAT, 0666);
	int retval = flock(lock_fd, LOCK_NB | LOCK_EX);
	if (retval != 0) assert(false);

	//
	int i = 0;
	while (true){
		string file_name("db." + to_string(i));
		boost_fs::path doc_path = current_path / file_name;

		int fd;
		if (boost_fs::exists(doc_path) && !boost_fs::is_directory(doc_path)){
			int fd, ret;
			fd = ::open(doc_path.c_str(), O_RDWR, 0644);
			auto meta = std::make_shared<file_meta>();
			meta->allocate = boost_fs::file_size(doc_path);
			fds.push_back(fd);
			metas.push_back(meta);
		}
		else{
			if (i == 0) create_file(doc_path);
			break;
		}
		i++;
	}
}

RC storage::create_file(boost_fs::path file_path){
	if (boost_fs::exists(file_path))assert(false);

	int fd, ret;
	fd = ::open(file_path.c_str(), O_RDWR | O_CREAT, 0644);

	auto meta = std::make_shared<file_meta>();
	meta->allocate = boost_fs::file_size(file_path);
	fds.push_back(fd);
	metas.push_back(meta);
}

RC storage::insert(bytes& rec, disk_loc& loc){
	int fd = fds.back();
	auto meta = metas.back();
	int retval = 0;

	//overflow
	if (meta->allocate > FILE_MAX_SIZE){
		string file_name("db." + to_string(fds.size()));
		boost_fs::path doc_path = current_path / file_name;
		create_file(doc_path);
		fd = fds.back();
		meta = metas.back();
	}

	retval = ::lseek(fd, meta->allocate, SEEK_SET);
	retval = ::write(fd, rec.data(), rec.length());
	if (retval != 0){
		loc.file_id = fds.size() - 1;
		loc.rec_id = meta->allocate;
		loc.rec_size = rec.length();
		meta->allocate += rec.length();
	}
	else throw;
}

RC storage::read(disk_loc& loc, bytes& rec){
	int fd = fds[loc.file_id];
	auto meta = metas[loc.file_id];

	char buffer[256] = { 0 };
	int read_len = 0;
	int ret = 0;

	ret = ::lseek(fd, loc.rec_id, SEEK_SET);
	ret = ::read(fd, buffer, loc.rec_size);
	rec.append(buffer, loc.rec_size);
	read_len += ret;
}