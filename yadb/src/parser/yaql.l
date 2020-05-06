%{
    #include<stdio.h>
	#include <parser/query.hpp>
    #include "yaql.tab.h"
    char *strclone(char *str);
%}
DIGIT1to9 [1-9]
DIGIT [0-9]
DIGITS {DIGIT}+
INT {DIGIT}|{DIGIT1to9}{DIGITS}|-{DIGIT}|-{DIGIT1to9}{DIGITS}
FRAC [.]{DIGITS}
EXP {E}{DIGITS}
E [eE][+-]?
HEX_DIGIT [0-9a-f]
NUMBER {INT}|{INT}{FRAC}|{INT}{EXP}|{INT}{FRAC}{EXP}
UNESCAPEDCHAR [ -!#-\[\]-~]
ESCAPEDCHAR \\["\\bfnrt/]
UNICODECHAR \\u{HEX_DIGIT}{HEX_DIGIT}{HEX_DIGIT}{HEX_DIGIT}
CHAR {UNESCAPEDCHAR}|{ESCAPEDCHAR}|{UNICODECHAR}
CHARS {CHAR}+
DBL_QUOTE ["]

%%
INSERT|insert { return INSERT; }
SELECT|select { return SELECT; }
FROM|from { return FROM; }
TO|to { return TO; }

{DBL_QUOTE}{DBL_QUOTE} |
{DBL_QUOTE}{CHARS}{DBL_QUOTE} {
	yaqllval.string= strdup(yaqltext);
    return STRING;
};
{NUMBER} {
	yaqllval.string= strdup(yaqltext);
    return NUMBER;
}
{NUMBER} {
    yaqllval.string= strdup(yytext);
    return NUMBER;
}
true { return _TRUE; };
false {return _FALSE; };
null { return _NULL; };
\{ { return O_BEGIN; };
\} { return O_END; };
\[ { return A_BEGIN; };
\] { return A_END; };
, { return COMMA; };
: { return COLON; };
[ \t\n]+ ; //ignore whitespace
. { return ERROR; }; //error token

%%
char *strclone(char *str)
{
    int len = strlen(str);
    char *clone = (char*)malloc(sizeof(char)*(len+1));
    strcpy(clone,str);
    return clone;
}

extern void scan_yaql(const char* str){
    yaql_scan_string(str);
}