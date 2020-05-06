#include "serv_raft.hpp"
#include "journal/journal_raft.hpp"
#include "proto/raft.pb.h"
#include "parser/query.hpp"
#include "storage/storage.hpp"
#include "serv/state_raft.hpp"
shared_ptr<serv_raft> cluster = nullptr;

//parser
void scan_yaql(const char*);
int yaqlparse(query* q);

/*
 * connection
 */
connection_raft::connection_raft(){
	DEBUG("+connection_raft");
	conn_reader = make_shared<reader_raft>();
}

RC connection_raft::read(){
	conn_reader->read(shared_from_base<connection_raft>());
}

RC connection_raft::dispatcher(bytes header, bytes body){
	reader_raft::header h;
	memcpy(&h, header.data(), sizeof(h));
	
	if (h.type == VOTE_REQ){
		raft_proto::vote_request req;
		raft_proto::vote_response resp;
		bool ret = req.ParseFromString(body);

		{
			lock_guard<mutex> lock(cluster->mutex4st);
			int vote_for = cluster->curr_vote;
			int curr_term = cluster->curr_term;
			int last_idx = journal->last();

			resp.set_term(curr_term);
			if (curr_term > req.term()) REJECT_VOTE(resp)
			else if (last_idx > req.last_log_idx()) REJECT_VOTE(resp)
			else if (curr_term == req.term() && vote_for != 0 && vote_for != req.candidate_id()) REJECT_VOTE(resp)
			else {
				ACCEPT_VOTE(resp)
				cluster->curr_vote = req.candidate_id();
				cluster->curr_term = req.term();
				cluster->raft_st->update_heartbeat();
			}
		}

		string resp_body;
		resp.SerializeToString(&resp_body);
		response(h.request_id, VOTE_RESP, resp_body);
	}
	else if (h.type == APPEND_REQ){
		raft_proto::append_entry_requset req;
		raft_proto::append_entry_response resp;
		bool ret = req.ParseFromString(body);
		cluster->raft_st->update_heartbeat();
		DEBUG(RED "serv_raft APPEND_REQ %d" NONE, req.etriies_size());

		int last_idx = journal->last();
		int prev_idx = req.prev_log_idx();
		DEBUG(RED "last_idx:%d,  prev_idx:%d" NONE, last_idx, prev_idx);
		if (req.prev_log_idx() <= last_idx){
			ACCEPT_APPEND(resp)
			for (int i = 0; i < req.etriies_size(); i++) {
				auto entry = req.etriies(i);
				bytes rec = entry.val();
				journal->insert_entry(prev_idx + i + 1, &rec);

				//index
				scan_yaql(rec.c_str());
				query q;
				int retval = yaqlparse(&q);
				if (retval == 0){
					if (q.cmd == QUERY_TYPE::INSERT){
						disk_loc loc;
						bytes rec = q.params["body"];
						int key = atoi(q.params["key"].c_str());
						store->insert(rec, loc);
						server->indexer->insert(key, loc);
					}
				}
			}
		}
		else REJECT_APPEND(resp)
		
		string resp_body;
		resp.set_term(0);
		resp.set_prev_log_idx(journal->curr_idx());
		resp.SerializeToString(&resp_body);
		response(h.request_id, VOTE_RESP, resp_body);
	}
	else if (h.type == VOTE_RESP || h.type == APPEND_RESP){
		DEBUG("serv_raft VOTE_RESP APPEND_RESP");
		auto r = request_queue.try_pop();
		r->callback(shared_from_base<connection_raft>(), body);
	}
	else{
		assert(false);
	}
}

RC connection_raft::request(RAFT_PROTO_TYPE type, string& body, raft_callback callback){
	static atomic<int> req_id(1);

	reader_raft::header header;
	header.body_size = body.size();
	header.type = type;
	header.request_id = req_id;

	raft_request req;
	req.request_id = req_id;
	req.callback = callback;
	request_queue.push(req);
	req_id++;

	int ctx_size = sizeof(reader_raft::header) + body.size();
	char* ctx = new char[ctx_size]();
	memcpy(ctx, &header, sizeof(reader_raft::header));
	memcpy(ctx + sizeof(reader_raft::header), body.data(), body.size());
	::write(conn_fd, ctx, ctx_size);
	delete[] ctx;
}

RC connection_raft::response(int request_id, RAFT_PROTO_TYPE type, string& body){
	reader_raft::header header;
	header.body_size = body.size();
	header.type = type;
	header.request_id = request_id;

	int ctx_size = sizeof(reader_raft::header) + body.size();
	char* ctx = new char[ctx_size]();
	memcpy(ctx, &header, sizeof(reader_raft::header));
	memcpy(ctx + sizeof(reader_raft::header), body.data(), body.size());
	::write(conn_fd, ctx, ctx_size);
	delete[] ctx;
}

/*
 * serv
 */
serv_raft::serv_raft(vector<pair<int, string>>* peers, int this_id) :
cluster_id(this_id),
curr_term(0),
curr_vote(0)
{
	fdes = new fd_events(3);
	for (auto itr : *peers) {
		if (itr.first == cluster_id) {
			vector<string> addr;
			boost::algorithm::split(addr, itr.second, boost::algorithm::is_any_of(":"));
			node_ip = addr[0];
			node_port = atoi(addr[1].c_str());
			continue;
		}
		this->peers.insert(itr.second, make_shared<connection_raft>());
	}
}

RC serv_raft::start(){
	server_conn = make_shared<connection_raft>();
	auto retval = connection::listen(node_ip, node_port, server_conn);
	assert(retval == RC::OK);
	fdes->set(server_conn);

	thread t_serv(&serv_raft::server_loop, this);
	t_serv.detach();

	vector<pair<string, shared_ptr<connection_raft>>> all_peers;
	peers.itr(&all_peers);

	for (int i = 0; i < 5; i++){
		bool peers_disconnect = false;
		for (auto itr : all_peers) {
			if (itr.second != nullptr && itr.second->is_alive()) continue;

			DEBUG("try connecting: %s", itr.first.c_str());
			vector<string> addr;
			boost::algorithm::split(addr, itr.first, boost::algorithm::is_any_of(":"));
			if (connection::connect(addr[0], atoi(addr[1].c_str()), fdes, itr.second) == RC::ERROR)
				peers_disconnect = true;
		}

		if (!peers_disconnect) break;
		this_thread::sleep_for(chrono::milliseconds(500));
	}

	is_init = true;
	raft_st = make_shared<raft_follower>();
	thread t_st(&state_raft::start, raft_st);
	t_st.detach();
}

RC serv_raft::accept(){
	DEBUG("server_raft accept");
	shared_ptr<connection_raft> cli_conn = make_shared<connection_raft>();
	server_conn->accept(cli_conn);
	fdes->set(cli_conn);

	if (!is_init) return RC::OK;
	vector<pair<string, shared_ptr<connection_raft>>> all_peers;
	peers.itr(&all_peers);


	for (auto itr : all_peers) {
		if (itr.second != nullptr && itr.second->is_alive()) continue;

		DEBUG("try re-connecting: %s", itr.first.c_str());
		vector<string> addr;
		boost::algorithm::split(addr, itr.first, boost::algorithm::is_any_of(":"));
		auto retval = connection::connect(addr[0], atoi(addr[1].c_str()), fdes, itr.second);
		if (retval == RC::OK) DEBUG(YELLOW "%s is reconnected" NONE, itr.first.c_str());
	}
}

RC serv_raft::get_active_peers(vector<shared_ptr<connection_raft>>* ret){
	auto filter = [](shared_ptr<connection_raft> conn){ return conn->is_alive(); };
	return peers.filter(filter, ret);
}
