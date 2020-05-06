#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <deque>
#include <mutex>
#include <string>
#include <time.h>
#include <thread>
#include "gtest/gtest.h"
#include "proto/raft.pb.h"
#include "serv/serv_raft.hpp"
using namespace std;
std::mutex m;
std::condition_variable cv;

const char* ip = "127.0.0.1";
int port = 5566;

//global
int resp_count;

TEST(dummy, dummy){}

/*
void start_serv(int cluster_id){
	vector<pair<int, string>> peers;
	peers.push_back(make_pair(1, "127.0.0.1:5566"));

	cluster = make_shared<serv_raft>(&peers, 1);
	cluster->start();
	cv.notify_all();
}

RC test_append_cb(shared_ptr<connection_raft> conn, string& body){
	resp_count++;
}

TEST(serv_raft, connect){
	thread t(start_serv, 1);
	std::unique_lock<std::mutex> lock(m);
	cv.wait(lock);

	fd_events fdes;
	shared_ptr<connection_raft> conn = make_shared<connection_raft>();
	connection_raft::connect(ip, port, &fdes, conn);

	int size = 1;
	for (int i = 0; i < size; i++){
		bytes body;
		auto callback = bind(test_append_cb, ARG1, ARG2);

		raft_proto::append_entry_requset req;
		req.set_term(i);
		req.set_leader_id(i);
		req.set_prev_log_idx(i);

		for (int j = 0; j < 5; j++){
			raft_proto::append_entry_requset::entry* entry = req.add_etriies();
			entry->set_idx(j);
			entry->set_val(to_string(j));
		}
		req.SerializeToString(&body);
		conn->request(RAFT_PROTO_TYPE::APPEND_REQ, body, callback);

		//read
		int n = -1;
		n = fdes.wait();
		conn->read();
	}

	EXPECT_EQ(resp_count, size);
	cluster->stop();
	t.join();
}
*/