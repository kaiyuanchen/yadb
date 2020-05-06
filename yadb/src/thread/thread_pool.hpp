#pragma once
#include "thread/thread_safe_queue.hpp"
#include "yadb.hpp"

template<class T>
class thread_pool {
public:
	thread_pool();
	RC start();
	RC stop();

private:
	enum class th_state {
		stoped,
		started
	};
	int th_num = 0;
	th_state state = th_state::stoped;
	threadsafe_queue<T> req_queue;
};