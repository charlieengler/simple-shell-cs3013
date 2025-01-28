%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lsh_ast.h"
#include "lsh.yacc.generated_h"
#include "lsh.lex.generated_h"


%}

%define api.pure full
%define parse.error detailed
%locations
%parse-param { struct context *context }
%parse-param { yyscan_t yyscanner }
%lex-param { yyscan_t yyscanner }

%code provides {

void yyerror (YYLTYPE *y, struct context *context, yyscan_t yyscanner, char const *s);

}

%start script_file


%token PIPE FOR IN DO PDO DONE IF THEN ELIF ELSE FI VAR WORD AMPERSAND SEMICOLON NEW_LINE VAR_ASSIGN OR AND LPAREN RPAREN

%union {
	struct script *script;
	struct statement *statement;
	struct program *program;
	struct words *words;
	struct word *word;
	struct for_loop *for_loop;
	struct conditional *conditional;
	struct var_assign *var_assign;
	char charval;
	char* strval;
}

%type <script> script script_file
%type <statement> statement fg_statement bg_statement;
%type <for_loop> for_loop
%type <conditional> conditional end_conditional
%type <var_assign> var_assign
%type <program> program programs and_programs or_programs pipe_programs
%type <words> words
%type <word> word
%type <charval> term terms
%type <strval> WORD VAR VAR_ASSIGN


%%                   /* beginning of rules section */

script_file:	YYEOF				{ context->script = NULL; }
	|	script YYEOF			{ context->script = $$ = $1; }
	|	script terms YYEOF		{ context->script = $$ = $1; }
	;

script:		statement			{ context->script = $$ = new_script(); if ($1 != NULL) { append_ll($$, $1); } }
	|	terms statement			{ context->script = $$ = new_script(); if ($2 != NULL) { append_ll($$, $2); } }
	|	script terms statement		{ context->script = $$ = $1; if ($3 != NULL) { append_ll($1, $3); } }
	;

statement:	fg_statement			{ $$ = $1; }
	|	bg_statement			{ $$ = $1; }
	;

bg_statement:	fg_statement AMPERSAND		{ $$ = $1; $$->background = 1; }
	;

fg_statement:	for_loop			{ $$ = new_statement(); $$->for_loop = $1; }
	|	conditional			{ $$ = new_statement(); $$->conditional = $1; }
	|	programs			{ $$ = new_statement(); $$->program = $1; }
	|	var_assign			{ $$ = new_statement(); $$->var_assign = $1; }
	;

for_loop:	FOR word IN terms DO script terms DONE		{ $$ = new_for_loop(); $$->var_name = $2; $$->script = $6; }
	|	FOR word IN words terms DO script terms DONE	{ $$ = new_for_loop(); $$->var_name = $2; $$->var_values = $4; $$->script = $7; }
	|	FOR word IN terms PDO script terms DONE		{ $$ = new_for_loop(); $$->var_name = $2; $$->script = $6; $$->parallel = 1; }
	|	FOR word IN words terms PDO script terms DONE	{ $$ = new_for_loop(); $$->var_name = $2; $$->var_values = $4; $$->script = $7; $$->parallel = 1; }
	;

conditional:	IF script terms THEN script terms end_conditional	{ $$ = $7; { struct conditional_part *cp = new_conditional_part(); cp->predicate = $2; cp->if_true_block = $5; prepend_ll($7, cp); } }
	;

end_conditional:  FI			{ $$ = new_conditional(); }
	|	 ELIF script terms THEN script terms end_conditional	{ $$ = $7; { struct conditional_part *cp = new_conditional_part(); cp->predicate = $2; cp->if_true_block = $5; prepend_ll($7, cp); } }
	|	 ELSE script terms FI		{ $$ = new_conditional(); $$->else_block = $2; }
	;

programs:	and_programs			{ $$ = $1; }
	;

and_programs:	or_programs			{ $$ = $1; }
	|	and_programs AND or_programs	{ $$ = new_program(); $$->run_fn = run_and_programs; $$->print_fn = print_and_programs; $$->lhs = $1; $$->rhs = $3; }
	;

or_programs:	pipe_programs			{ $$ = $1; }
	|	or_programs OR pipe_programs	{ $$ = new_program(); $$->run_fn = run_or_programs; $$->print_fn = print_or_programs; $$->lhs = $1; $$->rhs = $3; }
	;	
	
pipe_programs:	program				{ $$ = $1; }
	|	pipe_programs PIPE program	{ $$ = new_program(); $$->run_fn = run_pipe_programs; $$->print_fn = print_pipe_programs; $$->lhs = $1; $$->rhs = $3; }
	;

program:	words				{ $$ = new_program(); $$->words = $1; }
	|	LPAREN script RPAREN		{ $$ = new_program(); $$->script = $2; }
	;

words:		word				{ $$ = new_words(); append_ll($$, $1); }
	|	words word			{ $$ = $1; append_ll($1, $2); }
	;

var_assign:	VAR_ASSIGN word			{ $$ = new_var_assign(); char* s = $1; s[strlen(s) - 1] = 0; $$->var_name = s; $$->var_value = new_words(); append_ll($$->var_value, $2); }
	|	VAR_ASSIGN			{ $$ = new_var_assign(); char* s = $1; s[strlen(s) - 1] = 0; $$->var_name = s; $$->var_value = new_words(); }
	;

word:		WORD				{ $$ = new_word(); $$->text = $1; }
	|	VAR				{ $$ = new_word(); $$->text = $1; $$->is_var = 1; }
	;

terms:		term		{ $$ = $1; }
	|	terms term	{ $$ = $2; }
	;

term:		SEMICOLON	{ $$ = ';'; }
	|	NEW_LINE	{ $$ = '\n'; }
	;


%%

void yyerror (YYLTYPE *y, struct context *context, yyscan_t yyscanner, char const *s) {
	fprintf(stderr, "%s at line %d\n", s, yyget_lineno(yyscanner)); 
}

int yywrap(yyscan_t yyscanner)
{
	return 1;
}
