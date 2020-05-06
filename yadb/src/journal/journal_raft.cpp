#include "journal_raft.hpp"
#include <fcntl.h>
#include <iostream>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

journal_raft* journal = nullptr;

journal_raft::journal_raft(string path){
	journal_fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
}

RC journal_raft::insert_entry(bytes* src){
	return append(curr_man.curr_idx + 1, src);
}

RC journal_raft::insert_entry(int idx, bytes* src){
	return append(idx, src);
};

RC journal_raft::append(int idx, bytes* src){

	//seek
	char buff[JOURNAL_BLOCK_SIZE] = { 0 };
	int seek_idx = idx - 1;
	int read_pos = 0;
	int retval = 0;
	int buff_off = 0;
	int pos = 0;
	int remain_block_size = 0;
	bool is_found = false;

	while (!is_found){
		if (seek_idx == 0) break;
		if (::lseek(journal_fd, read_pos, SEEK_SET)< 0) return RC::ERROR;
		if (::read(journal_fd, buff, JOURNAL_BLOCK_SIZE)< 0) return RC::ERROR;

		buff_off = 0;
		while (buff_off + JOURNAL_SEG_HEADER_SIZE  < JOURNAL_BLOCK_SIZE){
			seg_header* header = reinterpret_cast<seg_header*>(buff + buff_off);
			buff_off += JOURNAL_SEG_HEADER_SIZE + header->len;  //skip to next header

			if (header->idx < seek_idx) continue;
			if (header->idx > seek_idx) return RC::ERROR;
			if (header->type == seg_type::FIRST || header->type == seg_type::MID) continue;  //hit but not last

			if (buff_off + JOURNAL_SEG_HEADER_SIZE > JOURNAL_BLOCK_SIZE) read_pos += JOURNAL_BLOCK_SIZE; //set pos to next block
			else read_pos += buff_off; //set to next pos
			pos = read_pos;
			is_found = true;
			break;
		}
		read_pos += JOURNAL_BLOCK_SIZE;
	}
	//DEBUG("append read_pos: %d", pos);

	//prune journal
	if (idx < curr_man.curr_idx){
		int ret = ftruncate(journal_fd, pos);
		curr_man.curr_idx = seek_idx;
	}

	//insert
	remain_block_size = JOURNAL_BLOCK_SIZE - (pos % JOURNAL_BLOCK_SIZE);
	int ptr = 0;
	const int total = src->size();
	const char* data = src->data();
	const int loop = (total + JOURNAL_SEG_HEADER_SIZE >= remain_block_size  && remain_block_size != 0) ?
		ceil((float)(total + JOURNAL_SEG_HEADER_SIZE - remain_block_size) / JOURNAL_BLOCK_DATA_SIZE) + 1 : 1;

	retval = ::lseek(journal_fd, pos, SEEK_SET);
	int remain_size = total;
	for (int i = 0; i < loop; i++){
		seg_header header;
		header.idx = idx;

		if (loop == 1){
			header.len = total;
			header.type = seg_type::FULL;
		}
		else if (i == 0){
			header.len = remain_block_size - JOURNAL_SEG_HEADER_SIZE;
			header.type = seg_type::FIRST;
		}
		else if (i == loop - 1){
			header.len = remain_size;
			header.type = seg_type::LAST;
		}
		else{
			header.len = JOURNAL_BLOCK_DATA_SIZE;
			header.type = seg_type::MID;
		}

		remain_size -= header.len;
		write(&header, data + ptr);
		ptr += header.len;
	}

	curr_man.curr_idx = idx;
	return RC::OK;
}

RC journal_raft::write(seg_header* header, const char* body){
	//DEBUG("write header: %d", (int)header->type);
	if (::write(journal_fd, header, JOURNAL_SEG_HEADER_SIZE)< 0) assert(false);
	if (::write(journal_fd, body, header->len)< 0) assert(false);
};

RC journal_raft::seek(int idx){
	if (idx > curr_man.curr_idx && idx == 0) return RC::ERROR;

	char buff[JOURNAL_BLOCK_SIZE] = { 0 };
	int read_pos = 0;
	while (1){
		if(::lseek(journal_fd, read_pos, SEEK_SET)< 0) assert(false);
		if (::read(journal_fd, buff, JOURNAL_BLOCK_SIZE)< 0) assert(false);
	}
}

RC journal_raft::get(int idx, bytes& ret){
	if (idx > curr_man.curr_idx || idx == 0) return RC::ERROR;

	static char buff[JOURNAL_BLOCK_SIZE] = { 0 };
	static int read_pos = 0;
	static int buff_off = 0;
	static int last_idx = 0;

	//reset to zero
	if (last_idx != idx || last_idx == 0){
		read_pos = 0;
		buff_off = 0;
		if (::lseek(journal_fd, read_pos, SEEK_SET)< 0) return RC::ERROR;
		if (::read(journal_fd, buff, JOURNAL_BLOCK_SIZE)< 0)  return RC::ERROR;
	}
	else{
		if (::lseek(journal_fd, read_pos, SEEK_SET)< 0) return RC::ERROR;
		if (::read(journal_fd, buff, JOURNAL_BLOCK_SIZE)< 0)  return RC::ERROR;
	}

	while (1){
		//search in block
		while (buff_off + JOURNAL_SEG_HEADER_SIZE < JOURNAL_BLOCK_SIZE){
			seg_header* header = reinterpret_cast<seg_header*>(buff + buff_off);
			//DEBUG("read header->type: %d", (int)header->type);
			//DEBUG("read header->idx: %d", (int)header->idx);
			
			assert(header->idx != 0);
			if (header->idx < idx){  //seek next
				buff_off += JOURNAL_SEG_HEADER_SIZE + header->len;
				continue;
			}
			else if (header->idx > idx) return RC::ERROR;

			if (header->type == seg_type::FIRST || header->type == seg_type::MID){
				ret.append(buff + buff_off + JOURNAL_SEG_HEADER_SIZE, header->len);
				buff_off += JOURNAL_SEG_HEADER_SIZE + header->len;
				continue;
			}

			ret.append(buff + buff_off + JOURNAL_SEG_HEADER_SIZE, header->len);
			last_idx = header->idx;
			return RC::OK;
		}

		read_pos += JOURNAL_BLOCK_SIZE;
		buff_off = 0;
		//DEBUG("read read_pos: %d", read_pos);
		if (::lseek(journal_fd, read_pos, SEEK_SET)< 0) return RC::ERROR;
		if (::read(journal_fd, buff, JOURNAL_BLOCK_SIZE)< 0) return RC::ERROR;
	}
}

RC journal_raft::after(int idx, int size, vector<pair<int, bytes>>& ret){
	if (idx > curr_man.curr_idx) return RC::ERROR;
	if (idx == 0) idx = 1;
	int itr = 0;

	while(1){
		if (size > 0 && itr >= size) break;

		bytes rec;
		if (get(idx, rec) == RC::ERROR) break;
		ret.push_back(make_pair(idx, rec));
		idx++;
		itr++;
	}
	return RC::OK;
};

int journal_raft::last(){
	return curr_man.curr_idx;
};
