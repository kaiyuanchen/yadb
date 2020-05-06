#include <memory>
#include <vector>
#include "proto/raft.pb.h"
#include "serv/serv.hpp"
#include "serv/serv_raft.hpp"
#include "serv/state.hpp"
#include "yadb.hpp"
using namespace std;
using namespace chrono;

#define HEARTBEAT_TIMEOUT 3000
#define QUORUM_TIMEOUT 4000
#define HEARTBEAT_INTV 1000
#define FOLLOW_ITV 100

#define VOTE_REQ RAFT_PROTO_TYPE::VOTE_REQ
#define VOTE_RESP RAFT_PROTO_TYPE::VOTE_RESP
#define APPEND_REQ RAFT_PROTO_TYPE::APPEND_REQ
#define APPEND_RESP RAFT_PROTO_TYPE::APPEND_RESP

#define VOTE_OK raft_proto::vote_response::RESULT_TYPE::vote_response_RESULT_TYPE_OK
#define APPEND_OK raft_proto::append_entry_response::RESULT_TYPE::append_entry_response_RESULT_TYPE_OK
#define APPEND_REJECT raft_proto::append_entry_response::RESULT_TYPE::append_entry_response_RESULT_TYPE_REJECT

#define REJECT_VOTE(pbuf) { pbuf.set_result(raft_proto::vote_response::RESULT_TYPE::vote_response_RESULT_TYPE_REJECT); DEBUG(YELLOW "reject vote" NONE); }
#define ACCEPT_VOTE(pbuf) { pbuf.set_result(raft_proto::vote_response::RESULT_TYPE::vote_response_RESULT_TYPE_OK); DEBUG(YELLOW "accept vote" NONE);  }
#define REJECT_APPEND(pbuf) { pbuf.set_result(APPEND_REJECT); DEBUG(YELLOW "reject append" NONE); }
#define ACCEPT_APPEND(pbuf) { pbuf.set_result(APPEND_OK); DEBUG(YELLOW "accept append" NONE); }

/*
* leader
*/
class raft_leader : public state_raft{
public:
	raft_leader();
	RC start();
	RC append(vector<string>* recs);
private:
	int leader_term;
	mutex mutex4append;
	RC append_callback(shared_ptr<connection_raft> conn, string& body);
	RC get_reqest(connection_raft* conn, string* body);
	RC commit();
};

/*
* candidate
*/
class raft_candidate : public state_raft{
public:
	raft_candidate();
	RC start();
private:
	atomic<int> quorum_num;
	atomic<int> success;
	atomic<int> fails;

	int request_term;
	mutex mutex4stop;
	RC get_reqest(string* body);
	RC quorum();
	RC quorum_callback(shared_ptr<connection_raft> conn, string& body);
};

/*
* follower
*/
class raft_follower : public state_raft{
public:
	raft_follower();
	RC start();
private:
	mutex mutex4heartbeat;
	bool is_timeout();
};
