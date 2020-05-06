#include "yadb.hpp"
#include <chrono>
#include <string>
#include <thread>
#include <boost/program_options.hpp>
#include "journal/journal_raft.hpp"
#include "storage/storage.hpp"
#include "index/bpt_index.hpp"
#include "serv/serv.hpp"
#include "serv/serv_raft.hpp"

#define PO_INVAILD po::validation_error::invalid_option_value

using namespace std;
namespace po = boost::program_options;

int main(int argc, char *argv[]){

	po::options_description desc("yadb options");
	desc.add_options()
		//("cli", po::value<string>(), "client ip:port")
		("cluster", po::value<string>(), "cluster op:port");

	string cli_addr;
	string cluster_addr;
	po::variables_map vm;
	try
	{
		po::store(po::parse_command_line(argc, argv, desc), vm);
		//if (vm.count("cli")) cli_addr = vm["cli"].as<string>();
		//else throw po::validation_error(PO_INVAILD, "cli");

		if (vm.count("cluster")) cluster_addr = vm["cluster"].as<string>();
		else throw po::validation_error(PO_INVAILD, "cluster");

		po::notify(vm);
	}
	catch (po::error& e){
		cout << e.what() << endl;
		cout << endl;
		cout << desc << endl;
		assert(false);
	}

	vector<pair<int, string>> peers;
	peers.push_back(make_pair(1, "127.0.0.1:5566"));
	peers.push_back(make_pair(2, "127.0.0.1:7788"));
	peers.push_back(make_pair(3, "127.0.0.1:5678"));

	int current_id = 0;
	for (auto itr : peers)
		if (itr.second == cluster_addr)
			current_id = itr.first;
	assert(current_id != 0);

	//server
	if (cluster_addr == "127.0.0.1:5566") server = new serv("127.0.0.1", 15566);
	else if (cluster_addr == "127.0.0.1:7788") server = new serv("127.0.0.1", 17788);
	else if (cluster_addr == "127.0.0.1:5678")  server = new serv("127.0.0.1", 15678);

	//create jounral
	store = new storage();
	server->indexer = make_shared<bpt_index<int, disk_loc>>();

	if (cluster_addr == "127.0.0.1:5566"){
		journal = new journal_raft("5566.log");
		server->indexer->start("5566/5566.idx");
		store->start("5566");
	}
	else if (cluster_addr == "127.0.0.1:7788"){
		journal = new journal_raft("7788.log");
		server->indexer->start("7788/7788.idx");
		store->start("7788");
	}
	else if (cluster_addr == "127.0.0.1:5678"){
		journal = new journal_raft("5678.log");
		server->indexer->start("5678/5678.idx");
		store->start("5678");
	}

	//run cluster
	cluster = make_shared<serv_raft>(&peers, current_id);
	thread cl_t(&serv_raft::start, cluster);
	cl_t.detach();

	while (true) this_thread::sleep_for(chrono::minutes(5566));
}
