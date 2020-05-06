#include "bpt_fs.hpp"

bpt_fs::bpt_fs(){
	//cache = new char[BPT_CACHE_SIZE* BPT_BLOCK_SIZE]();
	//cache_ids = new int[BPT_CACHE_SIZE]();
	//std::fill_n(cache, BPT_CACHE_SIZE* BPT_BLOCK_SIZE, 0);
	//std::fill_n(cache_ids, BPT_BLOCK_SIZE, -1);
}

bpt_fs::~bpt_fs(){
	//delete cache;
	//delete cache_ids;
}

RC bpt_fs::open(const char* path){
	fd = ::open(path, O_RDWR | O_CREAT, 0644);
	if (fd < 0) return RC::ERROR;
	return RC::OK;
}

RC bpt_fs::seek(int32_t node_id){
	size_t seek_off = node_id* BPT_BLOCK_SIZE+ sizeof(bpt_header);
	int retval = ::lseek(fd, seek_off, SEEK_SET);
	if (retval < 0) return  RC::ERROR;
	return RC::OK;
}

RC bpt_fs::read(int32_t node_id, void* block, int32_t block_size){
	//form cache
	//int hash_id = node_id% BPT_CACHE_SIZE;
	//if (*(cache_ids + hash_id) == node_id){
	//	memcpy(block, cache + hash_id* BPT_BLOCK_SIZE, block_size);
	//	return RC::OK;
	//}

	seek(node_id);
	int retval = ::read(fd, block, block_size);
	if(retval < 0) return  RC::ERROR;

	//memcpy(cache + hash_id* BPT_BLOCK_SIZE, block, block_size);
	//*(cache_ids+ hash_id) = node_id;
	return RC::OK;
}

RC bpt_fs::write(int32_t node_id, void* block, int32_t block_size){
	seek(node_id);
	int retval = ::write(fd, block, block_size);
	if (retval < 0) return  RC::ERROR;

	//cache
	//int hash_id = node_id% BPT_CACHE_SIZE;
	//memcpy(cache + hash_id* BPT_BLOCK_SIZE, block, block_size);
	//*(cache_ids + hash_id) = node_id;
	return RC::OK;
}

size_t bpt_fs::get_size(){
	struct stat fd_stat;
	int ret = ::fstat(fd, &fd_stat);
	return fd_stat.st_size;
}

RC bpt_fs::read_header(void* header, size_t block_size){
	int retval = ::lseek(fd, 0, SEEK_SET);
	retval = ::read(fd, header, block_size);
	if (retval < 0) return  RC::ERROR;
	return RC::OK;
}

RC bpt_fs::write_header(void* header, size_t block_size){
	int retval = ::lseek(fd, 0, SEEK_SET);
	retval = ::write(fd, header, block_size);
	if (retval < 0) return  RC::ERROR;
	return RC::OK;
}