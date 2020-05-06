#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <vector>
#include "serv/serv.hpp"
using namespace std;
         
#define SERVER_ADDR     "127.0.0.1"  
FILE *infile, *outfile;

class connection_client : public connection{
public:
	RC read(){
		conn_reader->read(shared_from_base<connection_client>());
	}

private:
	SHARED_FROM_BASE
	RC dispatcher(bytes header, bytes body){
		cout << LIGHT_CYAN << body << NONE << endl;
	}
};

int main()
{
	vector<pair<string, int>> peers;
	peers.push_back(make_pair("127.0.0.1", 15566));
	peers.push_back(make_pair("127.0.0.1", 17788));
	peers.push_back(make_pair("127.0.0.1", 15678));

	while (true) {
		char cmd[100] = { 0 };
		cout << "yadb> ";
		std::string line;
		std::getline(std::cin, line);

		auto curr_conn = make_shared<connection_client>();
		fd_events fdes;
		bool is_conn = false;
		for (auto peer : peers){
			if (connection::connect(peer.first, peer.second, &fdes, curr_conn) == RC::OK){
				DEBUG("connect: %s:%d", peer.first.c_str(), peer.second);
				cout << YELLOW "request@" << peer.first.c_str() << peer.second << NONE << endl;
				is_conn = true;
				break;
			};
		}
		assert(is_conn);

		//request
		//int len = strlen(cmd) - 1;
		//if (cmd[len] = '\n') cmd[len] = '\0';
		curr_conn->request(line);

		//read
		fdes.wait();
		curr_conn->read();
	}

	return 0;
}
