%{

#include <stdio.h>
#include "lsh_ast.h"
#include "lsh.yacc.generated_h"

int prev2_tok = -1;
int prev_tok = -1;

void set_prev(int this_tok) {
	prev2_tok = prev_tok;
	prev_tok = this_tok;
}

int is_keyword(int tok) {
	switch (tok) {
		case FOR:
		case IN:
		case DO:
		case PDO:
		case DONE:
		case IF:
		case THEN:
		case ELIF:
		case ELSE:
		case FI:
			return 1;
		default:
			return 0;
	}
}

#define SET_PREV_AND_RETURN(tok)	do { set_prev(tok); return tok; } while(0)
#define KEYWORD_IF_FIRST(tok)		do {	\
	if (prev_tok < 0 || prev_tok == NEW_LINE || prev_tok == SEMICOLON || is_keyword(prev_tok)) {	\
		SET_PREV_AND_RETURN(tok);			\
	} else {						\
		yylval->strval = strdup(yytext);		\
		SET_PREV_AND_RETURN(WORD);			\
	}							\
} while(0)

%}

%option reentrant
%option bison-bridge
%option bison-locations
%option yylineno

%option header-file="lsh.lex.generated_h"

%%

[ \t]+		{ ; }
\(		{ SET_PREV_AND_RETURN(LPAREN); }
\)		{ SET_PREV_AND_RETURN(RPAREN); }
\|		{ SET_PREV_AND_RETURN(PIPE); }
\|\|		{ SET_PREV_AND_RETURN(OR); }
\;		{ SET_PREV_AND_RETURN(SEMICOLON); }
\n		{ SET_PREV_AND_RETURN(NEW_LINE); }
\&		{ SET_PREV_AND_RETURN(AMPERSAND); }
\&\&		{ SET_PREV_AND_RETURN(AND); }

for		{ KEYWORD_IF_FIRST(FOR); }
in		{ if (prev2_tok == FOR) { SET_PREV_AND_RETURN(IN); } else { yylval->strval = strdup(yytext); SET_PREV_AND_RETURN(WORD); } }
do		{ KEYWORD_IF_FIRST(DO); }
pdo		{ KEYWORD_IF_FIRST(PDO); }
done		{ KEYWORD_IF_FIRST(DONE); }
if		{ KEYWORD_IF_FIRST(IF); }
then		{ KEYWORD_IF_FIRST(THEN); }
elif		{ KEYWORD_IF_FIRST(ELIF); }
else		{ KEYWORD_IF_FIRST(ELSE); }
fi		{ KEYWORD_IF_FIRST(FI); }

[$][a-zA-Z_][a-zA-Z0-9_]*	{ yylval->strval = strdup(yytext+1); SET_PREV_AND_RETURN(VAR); }
[a-zA-Z0-9_\-\.^$/*]+		{ yylval->strval = strdup(yytext); SET_PREV_AND_RETURN(WORD); }
[a-zA-Z_][a-zA-Z0-9_]*=		{ yylval->strval = strdup(yytext); SET_PREV_AND_RETURN(VAR_ASSIGN); }
\'[^']*\'			{ yylval->strval = strdup(yytext+1); {int sl = strlen(yylval->strval); if (sl > 0) yylval->strval[sl - 1] = 0; } SET_PREV_AND_RETURN(WORD); }

.		{ fprintf(stderr, "bad input character '%s' at line %d\n", yytext, yylineno); SET_PREV_AND_RETURN(YYEOF); }


%%
