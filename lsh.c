#include <stdio.h>
#include <string.h>
#include <search.h>
#include <unistd.h>
#include <getopt.h>

#include <readline/readline.h>

#include "lsh_ast.h"
#include "lsh.yacc.generated_h"
#include "lsh.lex.generated_h"

#define PROMPT	"$ "

// man 7 environ
extern char **environ;

int print_ast = 0;
int print_ast_only = 0;

int handle_script(struct context *context) {
	if (context->script) {
		if (print_ast || print_ast_only) {
			print_script(stdout, context->script, 0);
		}
		if (print_ast_only) {
			return 0;
		}

		struct run_context run_context = DEFAULT_RUN_CONTEXT;
		run_script(context, context->script, &run_context);	

		free_script(context->script);
		context->script = NULL;
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct context *context = new_context();
	int rc;
	FILE *finput = NULL;
	yyscan_t scanner;

	// Load environment into a data structure. These will work as variables for
	// variable expansion, for example 'echo $HOME'.
	for (char **p = environ; p && *p; p++) {
		const char *buf = strdup(*p);
		void *t = tsearch(buf, &context->env_tree, env_tree_compare);
		if (buf != *(const char **)t) free((void*)buf);
	}
	//twalk(context->env_tree, tsearch_print_env_tree);


	// argument parsing.
	while (1) {
		//int this_option_optind = optind ? optind : 1;
		int c;
		int option_index = 0;
		static struct option long_options[] = {
			{"print_ast",		no_argument,	0, 0 },
			{"print_ast_only",	no_argument,	0, 0 },
			{"yydebug",		no_argument,	0, 0 },
			{0, 0, 0, 0 }
		};

		c = getopt_long(argc, argv, "",
				long_options, &option_index);
		//printf("c is %d option_index %d argc %d argv %p optind %d\n", c, option_index, argc, argv[0], optind);
		if (c == -1)
			break;

		switch (c) {
			case 0:
				switch (option_index) {
					case 0:
						print_ast = 1;
						break;
					case 1:
						print_ast_only = 1;
						break;
					case 2:
						yydebug = 1;
						break;
				}
				break;
		}
	}

	yylex_init(&scanner);

	if (argc == optind && isatty(0)) {
		// If stdin is a terminal, and no arguments are specified, assume an interactive terminal is desired.
		// Use readline() to provide a pleasant-ish experience.
		char *input;
		yylex_init(&scanner);
		while ((input = readline(PROMPT)) != NULL) {
			yy_switch_to_buffer(yy_scan_string(input, scanner), scanner);
			if ((rc = yyparse(context, scanner)) == 0) {
				rc = handle_script(context);
			}
			free(input);
		}
	} else {
		// Read from a script. By default this is stdin.
		if (argc > optind) {
			// If a file is specified as a command line argument, read from that instead of stdin.
			const char *source = argv[optind];
			finput = fopen(source, "rb");
			if (finput == NULL) {
				fprintf(stderr, "Could not open '%s' for reading, errno %d (%s)\n", source, errno, strerror(errno));
				return 1;
			}
			yyset_in(finput, scanner);
		}
		// Parse the input file and run the parsed script if parsing was successful.
		if ((rc = yyparse(context, scanner)) == 0) {
			rc = handle_script(context);
		}
	}
	// Cleanup.
	yylex_destroy(scanner);
	if (finput) fclose(finput);
	free_context(context);
	return rc;
}

