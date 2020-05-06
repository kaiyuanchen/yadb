#pragma once

#include <boost/algorithm/string.hpp>
#include <thread>
#include <vector>
#include "serv/serv.hpp"
#include "serv/state.hpp"
#include "thread/thread_safe_queue.hpp"
#include "thread/thread_safe_map.hpp"
using namespace std;
class raft_leader;
class raft_candidate;
class raft_follower;

enum class RAFT_PROTO_TYPE{
	VOTE_REQ,
	VOTE_RESP,
	APPEND_REQ,
	APPEND_RESP
};

/*
 * reader
 */
class reader_raft : public reader {
public:
	struct header{
		int body_size = 0;
		int request_id = 0;
		RAFT_PROTO_TYPE type;
	};

	explicit reader_raft(){ DEBUG("+reader_raft"); };
	~reader_raft(){ DEBUG("~reader_raft"); };
private:
	header rpc_header;
	header curr_header;

	virtual RC read_header(){
		memcpy(&curr_header, buf + header_ptr, header_size());
	};
	virtual RC read_body(bytes& body){
		body.append(buf + header_ptr + header_size(), body_size());
	};
	virtual int header_size() const { return sizeof(header); };
	virtual int body_size() const { return curr_header.body_size; };
};

/*
 * connect
 */
class connection_raft : public connection {
	friend raft_leader;

public:
	SHARED_FROM_BASE
	typedef function<RC(shared_ptr<connection_raft>, string&)> raft_callback;
	struct raft_request {
		int request_id = 0;
		raft_callback callback;
	};

	connection_raft();
	~connection_raft(){ DEBUG("~connection_raft %d", (int)conn_fd); };
	RC read();
	RC request(RAFT_PROTO_TYPE type, string& body, raft_callback callback);
	RC response(int request_id, RAFT_PROTO_TYPE type, string& body);
	
	
private:
	int next_index = 0;
	shared_ptr<reader_raft> conn_reader;
	threadsafe_queue<raft_request> request_queue;
	RC dispatcher(bytes header, bytes body);
	inline int get_next_index(){ return next_index; }
	inline RC set_next_index(int i){ next_index = i; }
};

/*
 * serv
 */
class serv_raft : public serv{
	friend connection;
	friend connection_raft;
	friend raft_leader;
	friend raft_candidate;
	friend raft_follower;

public:
	atomic<int> curr_term;
	atomic<int> curr_vote;
	
	serv_raft(vector<pair<int, string>>* peers, int this_id);
	RC start();
	RC stop(){};
	
	//raft
	template<class T>
	RC update_state(){
		if (raft_st != nullptr) raft_st->stop();
		raft_st = make_shared<T>();
		thread t(&T::start, static_pointer_cast<T>(raft_st));
		t.detach();
	};
private:
	bool is_init = false;
	const int cluster_id;
	threadsafe_map<string, shared_ptr<connection_raft>> peers;
	shared_ptr<state_raft> raft_st;
	mutex mutex4st;
	RC accept();
	RC add_peer();
	RC get_active_peers(vector<shared_ptr<connection_raft>>* ret);
	inline RC vote_self(){ curr_vote = cluster_id; };
	inline int get_id(){ return cluster_id; };
	inline int get_quorm(){ return ((peers.size() + 1) / 2) + 1; };
};

extern shared_ptr<serv_raft> cluster;
