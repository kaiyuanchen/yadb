%require "3.0"

%{
#include <cstdio>
#include <cstring>
#include <climits>
#include <iostream>
#include <sys/times.h>
#include <string>
#include <unistd.h>
#include <parser/query.hpp>
using namespace std;

int yaqllex(void);  
void yaqlerror(query *q, const char *str) { /*fprintf(stderr, "Error: %s\n", str);*/ }
extern "C" {int yaqlwrap() { return 1; }}
%}

%parse-param {query *q}
%union {
	char* string;
}

%token INSERT SELECT FROM TO
%token <string> STRING NUMBER ERROR
%left _TRUE _FALSE _NULL
%left O_BEGIN O_END A_BEGIN A_END
%left COMMA
%left COLON

//define rule reutrn type
%type <string> OBJECT ARRAY PAIR COMMA VALUE COLON MEMBERS ELEMENTS

%%
CMDS: CMD_INSERT| CMD_SELECT| CMD_SELECT_RANGE;
CMD_INSERT:	INSERT OBJECT { 
				q->cmd= QUERY_TYPE::INSERT;
				q->params.insert(make_pair("body", $2));
			};
CMD_SELECT:	SELECT STRING {
				q->cmd= QUERY_TYPE::SELECT;
				q->params.insert(make_pair("eq", string($2+1, strlen($2)-2)));
			};
		 
CMD_SELECT_RANGE: SELECT FROM STRING TO STRING {
				q->cmd= QUERY_TYPE::SELECT_RANGE;
				q->params.insert(make_pair("from", string($3+1, strlen($3)-2)));
				q->params.insert(make_pair("to", string($5+1, strlen($5)-2)));
			};

OBJECT: O_BEGIN O_END { $$ = "{}"; }|
		O_BEGIN MEMBERS O_END {
			$$ = (char *)malloc(sizeof(char)*(1+strlen($2)+1+1));
			sprintf($$,"{%s}",$2);
		};
MEMBERS: PAIR { $$ = $1; }| 
		 PAIR COMMA MEMBERS {
			$$ = (char *)malloc(sizeof(char)*(strlen($1)+1+strlen($3)+1));
			sprintf($$,"%s,%s",$1,$3);
		 };
PAIR:	STRING COLON VALUE {
			if(strcmp($1, "\"key\"") ==0) q->params.insert(make_pair("key", string($3+1, strlen($3)-2)));
			$$ = (char *)malloc(sizeof(char)*(strlen($1)+1+strlen($3)+1));
			sprintf($$,"%s:%s",$1,$3);
		};

VALUE:	STRING {$$=yaqllval.string;}| 
		NUMBER {$$=yaqllval.string;}| 
		OBJECT {$$=$1;}|
		ARRAY {$$=$1;}|
		_TRUE {$$="true";}| 
		_FALSE {$$="false";}| 
		_NULL {$$="null";};

ARRAY:	A_BEGIN A_END {
			$$ = (char *)malloc(sizeof(char)*(2+1));
			sprintf($$,"[]");
		}|
		A_BEGIN ELEMENTS A_END {
			$$ = (char *)malloc(sizeof(char)*(1+strlen($2)+1+1));
			sprintf($$,"[%s]",$2);
		};
ELEMENTS:	VALUE { $$ = $1; }| 
			VALUE COMMA ELEMENTS {
				$$ = (char *)malloc(sizeof(char)*(strlen($1)+1+strlen($3)+1));
				sprintf($$,"%s,%s",$1,$3);
			};
