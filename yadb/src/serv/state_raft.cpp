#include "state_raft.hpp"
#include "journal/journal_raft.hpp"
#include <thread>
#include <vector>

state_raft::state_raft(STATE_TYPE st) :
curr_st(st),
is_stop(true)
{
}

RC state_raft::update_heartbeat(){
	lock_guard <mutex> lock(mutex4heartbeat);
	heartbeat = NOW;
};

/*
* leader
*/
raft_leader::raft_leader() : state_raft(STATE_TYPE::LEADER){
	DEBUG(LIGHT_CYAN "raft_leader start" NONE);
	leader_term = ++cluster->curr_term;
	is_stop = false;

	vector<pair<string, shared_ptr<connection_raft>>> all_peers;
	cluster->peers.itr(&all_peers);
	for (auto itr : all_peers) itr.second->set_next_index(journal->curr_idx());
}

RC raft_leader::start(){
	thread cli_t(&serv::start, server);
	cli_t.detach();

	int i = 0;
	while (!is_stop) {
		append(nullptr);
		this_thread::sleep_for(chrono::milliseconds(1000));
	}

	//stop
	server->stop();
}

RC raft_leader::append(vector<string>* recs){
	lock_guard <mutex> lock(mutex4append);
	if (recs)
		for (auto rec : *recs) journal->insert_entry(&rec);

	vector<shared_ptr<connection_raft>> peers;
	cluster->get_active_peers(&peers);
	for (auto peer : peers) {
		string body;
		auto retval = get_reqest(peer.get(), &body);
		auto callback = bind(&raft_leader::append_callback, shared_from_base<raft_leader>(), ARG1, ARG2);
		peer->request(APPEND_REQ, body, callback);
	}
}

RC raft_leader::append_callback(shared_ptr<connection_raft> conn, string& body){
	raft_proto::append_entry_response resp;
	resp.ParseFromString(body);
	conn->set_next_index(resp.prev_log_idx());
	commit();
	if (resp.result() == APPEND_REJECT) append(nullptr);
}

RC raft_leader::get_reqest(connection_raft* conn, string* body){
	int next_id = conn->get_next_index();
	raft_proto::append_entry_requset req;
	req.set_term(leader_term);
	req.set_leader_id(cluster->get_id());
	req.set_prev_log_idx(next_id);

	vector<pair<int, string>> recs;
	journal->after(next_id + 1, 0, recs);

	for (auto rec : recs){
		raft_proto::append_entry_requset::entry* entry = req.add_etriies();
		entry->set_idx(rec.first);
		entry->set_val(rec.second);
	}

	req.SerializeToString(body);
	return RC::OK;
}

RC raft_leader::commit(){
	vector<int> indexs;
	vector<pair<string, shared_ptr<connection_raft>>> all_peers;
	cluster->peers.itr(&all_peers);

	//get all peer index
	for (auto itr : all_peers) indexs.push_back(itr.second->get_next_index());
	indexs.push_back(journal->curr_idx());
	std::sort(indexs.begin(), indexs.end());

	//quorm commit
	int quorm = cluster->get_quorm();
	int commit_index = indexs[quorm - 1];
	DEBUG("COMMIT:%d", commit_index);
}

/*
* candidate
*/
raft_candidate::raft_candidate() : state_raft(STATE_TYPE::CANDIDATE){
	DEBUG(LIGHT_CYAN "raft_candidate %d start" NONE, cluster->get_id());
	cluster->vote_self();
	is_stop = false;
	success = 1;
	fails = 0;
	request_term = ++cluster->curr_term;
}

RC raft_candidate::start(){
	quorum();
	this_thread::sleep_for(chrono::milliseconds(QUORUM_TIMEOUT));

	lock_guard <mutex> lock(mutex4stop);
	if (is_stop) return RC::OK;
	DEBUG(LIGHT_CYAN "raft_candidate timeout" NONE);
	this->stop();
	cluster->update_state<raft_follower>();
}

RC raft_candidate::get_reqest(string* body){
	raft_proto::vote_request req;
	req.set_candidate_id(cluster->get_id());
	req.set_term(request_term);
	req.set_last_log_idx(journal->last());
	req.SerializeToString(body);
	return RC::OK;
}

RC raft_candidate::quorum(){
	string body;
	auto retval = get_reqest(&body);
	auto callback = bind(&raft_candidate::quorum_callback, shared_from_base<raft_candidate>(), ARG1, ARG2);

	vector<shared_ptr<connection_raft>> peers;
	cluster->get_active_peers(&peers);
	quorum_num = cluster->get_quorm();
	for (auto peer : peers) peer->request(VOTE_REQ, body, callback);
}

RC raft_candidate::quorum_callback(shared_ptr<connection_raft> conn, string& body){
	raft_proto::vote_response resp;
	resp.ParseFromString(body);

	if (resp.result() == VOTE_OK){
		DEBUG("VOTE_OK");
		success++;
	}
	else{
		DEBUG("VOTE_REJECT");
		fails++;
	}

	lock_guard <mutex> lock(mutex4stop);
	if (is_stop) return RC::OK;
	if (success >= quorum_num) cluster->update_state<raft_leader>();
	else if (fails >= quorum_num) cluster->update_state<raft_follower>();
}


/*
 * follower
 */
raft_follower::raft_follower() : state_raft(STATE_TYPE::FOLLOWER){
	DEBUG(LIGHT_CYAN "raft_follower %d start" NONE, cluster->get_id());
}

RC raft_follower::start(){
	is_stop = false;
	update_heartbeat();

	while (!is_stop) {
		this_thread::sleep_for(chrono::milliseconds(FOLLOW_ITV));
		if (is_timeout()) cluster->update_state<raft_candidate>();
	}
}

bool raft_follower::is_timeout(){
	lock_guard <mutex> lock(mutex4heartbeat);
	auto duration = DURATION_MILLI(heartbeat);
	return (duration.count() > HEARTBEAT_TIMEOUT) ? true : false;
}

