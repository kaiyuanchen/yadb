#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include "yadb.hpp"
using namespace std;

#define JOURNAL_BLOCK_SIZE 50
#define JOURNAL_SEG_HEADER_SIZE sizeof(journal_raft::seg_header)
#define JOURNAL_BLOCK_DATA_SIZE (JOURNAL_BLOCK_SIZE- JOURNAL_SEG_HEADER_SIZE)

class journal_raft{
public:
	enum class seg_type{
		FULL,
		FIRST,
		MID,
		LAST
	};
	struct journal_manifest{
		int curr_idx = 0;
		int curr_commit = 0;
	};
	struct seg_header{
		int idx;
		int len;
		seg_type type;
	};
	struct record{
		int index = 0;
	};

	journal_raft(string path);
	RC insert_entry(bytes* src);
	RC insert_entry(int idx, bytes* src);
	RC get(int idx, bytes& ret);
	RC after(int idx, int size, vector<pair<int, string>>& ret);
	int last();
	int curr_idx(){ return curr_man.curr_idx; };

private:
	int man_fd;
	int journal_fd;
	journal_manifest curr_man;
	mutex mutex4update;

	RC wrtie_manifest();
	RC append(int idx, bytes* src);
	RC write(seg_header* header, const char* body);
	RC read(bytes& ret);
	RC seek(int32_t idx);
	RC prune(int32_t idx);
	size_t file_size();
};

extern journal_raft* journal;