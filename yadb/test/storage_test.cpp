#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <time.h>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>
#include <gtest/gtest.h>
#include "index/bpt_fs.hpp"
#include "index/bpt_index.hpp"
#include "index/bpt_index.cpp"
#include "index/bpt_node.hpp"
#include "index/bpt_node.cpp"
#include "storage/storage.cpp"
using namespace std;

struct test {
	int a = 0;
	int b = 1;
};

TEST(bpt, fs){
	auto f = make_shared<bpt_fs>();
	f->open("test_fs.idx");

	bpt_header header1;
	header1.index_allocated = 55688;
	f->write_header(&header1, sizeof(bpt_header));
	bpt_header header2;
	f->read_header(&header2, sizeof(bpt_header));
	EXPECT_EQ(header1.index_allocated, header2.index_allocated);

	auto bpt = make_shared<bpt_index<int, int>>();
	auto r = make_shared<index_node<int, int>>(1, false, bpt);
	shared_ptr<index_node<int, int>> foo = nullptr;
	r->set_root(1, 2, 5566);
	f->write(1, &r->node, sizeof(r->node));
	vector<string> keys1;
	for (int i = 0; i < r->order; i++) keys1.push_back(to_string(r->node.rec[i].key));
	string str1 = boost::algorithm::join(keys1, ",");

	auto w = make_shared<index_node<int, int>>(1, false, bpt);
	f->read(1, &w->node, sizeof(w->node));
	vector<string> keys2;
	for (int i = 0; i < r->order; i++) keys2.push_back(to_string(r->node.rec[i].key));
	string str2 = boost::algorithm::join(keys2, ",");
	EXPECT_EQ(str1, str2);
}

TEST(bpt, node_int_int){
	int size = 100;
	auto bpt = make_shared<bpt_index<int, int>>();
	bpt->start("test.index");
	for (int i = 0; i< size; i++) bpt->insert(i, 5000+ i);
	for (int i = 0; i < size; i++){
		int ret;
		EXPECT_EQ(bpt->search(i, &ret), RC::OK);
	}

	//search range
	vector<int> rets;
	bpt->search(10, 50 ,&rets);
	EXPECT_EQ(rets.size(), 41);

	bpt->commit();
	bpt->stop();
}

TEST(bpt, node_int_int_save){
	int size = 100;
	auto bpt = make_shared<bpt_index<int, int>>();
	bpt->start("test.index");
	
	for (int i = 0; i < size; i++){
		int ret;
		EXPECT_EQ(bpt->search(i, &ret), RC::OK);
	}

	for (int i = 500; i < 500 + size; i++) bpt->insert(i, 5000 + i);
	for (int i = 500; i < 500 + size; i++){
		int ret;
		EXPECT_EQ(bpt->search(i, &ret), RC::OK);
	}
	bpt->commit();
	bpt->stop();
}

TEST(bpt, node_int_disc_loc){
	int size = 100;
	auto bpt = make_shared<bpt_index<int, disk_loc>>();
	bpt->start("test_loc.index");

	for (int i = 0; i < size; i++){
		disk_loc loc;
		loc.file_id = i;
		loc.rec_id = i + 100;
		bpt->insert(i, loc);
	}

	for (int i = 0; i < size; i++) {
		disk_loc ret;
		EXPECT_EQ(bpt->search(i, &ret), RC::OK);
		EXPECT_EQ(ret.file_id, i);
	}

	bpt->commit();
	bpt->stop();
}

TEST(bpt, node_int_disc_loc_save){
	int size = 100;
	auto bpt = make_shared<bpt_index<int, disk_loc>>();
	bpt->start("test_loc.index");

	for (int i = 0; i < size; i++) {
		disk_loc ret;
		EXPECT_EQ(bpt->search(i, &ret), RC::OK);
		EXPECT_EQ(ret.file_id, i);
	}

	bpt->commit();
	bpt->stop();
}

TEST(storage, open){
	int size = 10;
	auto sto = make_shared<storage>();

	//init
	char buff[256];
	getcwd(buff, 255);
	boost_fs::path curr_path = buff;
	boost_fs::path sto_path = curr_path / "test_sto";

	boost::filesystem::create_directory(sto_path);
	sto->start(sto_path.string());

	vector<disk_loc> locs;
	for (int i = 0; i < size; i++){
		bytes rec = "12345";
		disk_loc loc;
		sto->insert(rec, loc);
		locs.push_back(loc);
	}

	for (auto loc : locs){
		bytes rec;
		bytes expect_rec = "12345";
		sto->read(loc, rec);
		EXPECT_EQ(rec, expect_rec);
	}
}
