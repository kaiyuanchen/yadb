#include <string>
#include <map>
using namespace std;

enum class QUERY_TYPE {
	INSERT,
	SELECT,
	SELECT_RANGE
};

struct query{
	QUERY_TYPE cmd;
	map<string, string> params;
};