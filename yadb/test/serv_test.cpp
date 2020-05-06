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
#include "serv/serv.hpp"
using namespace std;
std::mutex m;
std::condition_variable cv;

const char* ip = "192.168.1.102";
int port = 5566;

TEST(dummy, dummy){}

/*
void start_serv(shared_ptr<serv>& server){
	server = make_shared<serv>(ip, port);
	cv.notify_all();
	server->start();
	EXPECT_EQ(true, true);
}

void start_client(){
	auto s1 = make_shared<serv>(ip, port);
	s1->start();
	EXPECT_EQ(true, true);
}

TEST(serv, connect){
	EXPECT_EQ(true, true);

	shared_ptr<serv> server= nullptr;
	thread t(start_serv, std::ref(server));

	std::unique_lock<std::mutex> lock(m);
	cv.wait(lock);
	server->stop();

	fd_events fds;
	shared_ptr<connection> conn = make_shared<connection>();
	connection::connect(ip, port, &fds, conn);

	for (int i = 0; i < 10000; i++){
		bytes body = to_string(i);
		conn->request(body);
	}
	t.join();
}
*/