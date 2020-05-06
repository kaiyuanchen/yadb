#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <time.h>
#include "journal/journal_raft.hpp"
#include "gtest/gtest.h"
using namespace std;

std::string random_string(size_t length)
{
	auto randchar = []() -> char
	{
		const char charset[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
		const size_t max_index = (sizeof(charset) - 1);
		return charset[rand() % max_index];
	};
	std::string str(length, 0);
	std::generate_n(str.begin(), length, randchar);
	return str;
}

TEST(journal, insert){
	RC retval;
	journal_raft* journal = new journal_raft("test1.journal");

	//init
	{
		EXPECT_EQ(journal->last(), 0);

		vector<pair<int, string>> recs;
		retval = journal->after(1, 0, recs);
		EXPECT_EQ(retval, RC::ERROR);
	}

	//append_1
	{
		//append 
		bytes rec0 = "123456";
		journal->insert_entry(1, &rec0);
		EXPECT_EQ(journal->last(), 1);

		bytes ret0;
		journal->get(1, ret0);
		EXPECT_EQ(ret0, rec0);

		vector<pair<int, string>> recs;
		retval = journal->after(0, 0, recs);
		EXPECT_EQ(recs.size(), 1);
	}

	//append
	bytes rec1 = "55688";
	journal->insert_entry(&rec1);
	EXPECT_EQ(journal->last(), 2);

	bytes ret1;
	journal->get(2, ret1);
	EXPECT_EQ(ret1, "55688");

	//append with idx
	bytes rec2 = "7788";
	journal->insert_entry(1, &rec2);
	EXPECT_EQ(journal->last(), 1);

	bytes ret2;
	journal->get(1, ret2);
	EXPECT_EQ(ret2, "7788");

	//read overflow
	//bytes ret3;
	//retval = journal->get(8, ret3);
	//EXPECT_EQ(retval, RC::ERROR);

}

TEST(journal, insert_1000){
	journal_raft* journal = new journal_raft("test2.journal");
	deque<bytes> srcs;

	for (int i = 1; i <= 100; i++){
		bytes rec = to_string(i);
		journal->insert_entry(&rec);
		srcs.push_back(rec);
	}

	int i = 1;
	for (auto itr : srcs){
		bytes ret;
		journal->get(i, ret);
		EXPECT_EQ(itr, ret);
		i++;
	}

	//after
	vector<pair<int, bytes>> recs2;
	journal->after(5, 0, recs2);
	EXPECT_EQ(recs2.size(), 96);
	EXPECT_EQ(journal->last(), 100);
}

TEST(journal, insert_large){
	journal_raft* journal = new journal_raft("test3.journal");
	deque<bytes> srcs;

	for (int i = 1; i <= 100; i++){
		bytes rec = random_string(100);
		journal->insert_entry(&rec);
		srcs.push_back(rec);
	}

	int i = 1;
	for (auto itr : srcs){
		bytes ret;
		journal->get(i, ret);
		EXPECT_EQ(itr, ret);
		i++;
	}
}