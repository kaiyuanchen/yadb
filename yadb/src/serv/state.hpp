#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include "yadb.hpp"
using namespace std;

class state_raft : public enable_shared_from_this<state_raft>{
public:
	enum class STATE_TYPE{
		LEADER,
		CANDIDATE,
		FOLLOWER
	};

	state_raft(STATE_TYPE st);
	const STATE_TYPE curr_st;

	RC update_heartbeat();
	virtual RC start() = 0;
	inline RC stop(){ is_stop = true; };
protected:
	SHARED_FROM_BASE

	atomic<bool> is_stop;
	mutex mutex4heartbeat;
	system_clock::time_point heartbeat = NOW;
};
