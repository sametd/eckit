%{

/*
 * (C) Copyright 1996-2013 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <unistd.h>

#ifdef YYBISON
#define YYSTYPE_IS_DECLARED
int yylex();
/*int yydebug;*/
#endif

struct YYSTYPE {
    Cell* v;
    std::string str;
};

%}

%type <v>requests;

%type <v>request
%type <v>value
%type <v>values
%type <v>parameter
%type <v>parameters

%token <str>WORD
%token <str>STRING
%token <str>NUMB

%start all

%%

all:	 requests                     { RequestParserResult::result_ = $1; }
	;

requests : request                    { $$ = new Cell("_list", "", $1, 0); }
		 | requests request           { $$ = ($1)->append(new Cell("_list", "", $2, 0)); }

request : WORD                        { $$ = new Cell("_verb", $1, 0, 0); }
		| WORD ',' parameters         { $$ = new Cell("_verb", $1, 0, $3); }
		| WORD '.'                    { $$ = new Cell("_verb", $1, 0, 0); }
		| WORD ',' parameters '.'     { $$ = new Cell("_verb", $1, 0, $3); }
		;

parameters : parameter                { $$ = $1; }
		   | parameters ',' parameter { $$ = ($1)->append($3); }
		   ;

parameter : WORD '=' values           { $$ = new Cell("", $1, $3, 0); }
		  ;

values : value                        { $$ = new Cell("_list", "", $1, 0); }
	   | values '/' value             { $$ = ($1)->append(new Cell("_list", "", $3, 0)); }
	   ;

value: WORD                           { $$ = new Cell("", $1, 0, 0); } 
	 | STRING                         { $$ = new Cell("", $1, 0, 0); }
	 | NUMB                           { $$ = new Cell("", $1, 0, 0); }
     | '(' requests ')'               { $$ = $2; }
	 ;

%%
#include "requestl.c"

