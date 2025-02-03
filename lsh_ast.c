// Functions you need to implement are labeled below

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <search.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>

#include "lsh_ast.h"

// Forward declarations.
/*static*/ void context_pid_wait_tree_add(struct context *context, int pid);
/*static*/ void context_empty_pid_wait_tree(struct context *context);

int errno;

static int _env_tree_compare(const char *a, const char *b) {
	while (1) {
		if (a == b) return 0;
		int adone = a == 0 || *a == 0 || *a == '=';
		int bdone = b == 0 || *b == 0 || *b == '=';
		if (adone && bdone) return 0;
		if (bdone) return -1;
		if (adone) return 1;
		if (*a < *b) return -1;
		if (*a > *b) return 1;
		a++; b++;
	}
}

int env_tree_compare(const void *a, const void *b) {
	int rc = _env_tree_compare(a, b);
	return rc;
}

int pid_wait_tree_compare(const void *a, const void *b) {
	uintptr_t ap = (uintptr_t)a;
	uintptr_t bp = (uintptr_t)b;
	if (ap < bp) return -1;
	else if (ap == bp) return 0;
	else return -1;
}

void tsearch_print_env_tree(const void *nodep, VISIT which, int depth)
{
	const char *datap;
	(void)depth;
	datap = *(const char **) nodep;
	if (which == postorder || which == leaf) {
		printf("%d %p %s\n", which, nodep, datap);
	}
}


void space(FILE *f, int depth) {
	for (int i = 0; i < depth; i++)
		fprintf(f, "  ");
}

void print_words(FILE *f, const struct words *words) {
	int i = 0;
	for (const struct word *w = words->first; w != NULL; w = w->next) {
		fprintf(f, "%s%s", i++ ? " " : "", w->text);
	}
}

void print_or_programs(FILE *f, const struct program *program, int depth) {
	space(f, depth);
	fprintf(f, "or || programs:\n");
	print_program(f, program->lhs, depth + 1);
	print_program(f, program->rhs, depth + 1);
}

void print_and_programs(FILE *f, const struct program *program, int depth) {
	space(f, depth);
	fprintf(f, "and && programs:\n");
	print_program(f, program->lhs, depth + 1);
	print_program(f, program->rhs, depth + 1);
}

void print_pipe_programs(FILE *f, const struct program *program, int depth) {
	space(f, depth);
	fprintf(f, "pipe | programs:\n");
	print_program(f, program->lhs, depth + 1);
	print_program(f, program->rhs, depth + 1);
}

void print_program(FILE *f, const struct program *program, int depth) {
	if (program->print_fn) {
		program->print_fn(f, program, depth);
	} else if (program->script) {
		print_script(f, program->script, depth);
	} else {
		space(f, depth);
		fprintf(f, "program: ");
		print_words(f, program->words);
		fprintf(f, "\n");
	}
}

void print_conditional(FILE *f, const struct conditional *conditional, int depth) {
	int i = 0;
	for (const struct conditional_part *cp = conditional->first; cp != NULL; cp = cp->next) {
		space(f, depth);
		fprintf(f, "%sif:\n", i++ ? "el" : "");
		print_script(f, cp->predicate, depth + 1);
		space(f, depth);
		fprintf(f, "then:\n");
		print_script(f, cp->if_true_block, depth + 1);
	}
	if (conditional->else_block != NULL) {
		space(f, depth);
		fprintf(f, "else:\n");
		print_script(f, conditional->else_block, depth + 1);
	}	
}

void print_for_loop(FILE *f, const struct for_loop *for_loop, int depth) {
	space(f, depth);
	fprintf(f, "for %s in ", for_loop->var_name->text);
	print_words(f, for_loop->var_values);
	fprintf(f, "; %sdo\n", for_loop->parallel ? "parallel " : "");
	print_script(f, for_loop->script, depth + 1);
}

void print_var_assign(FILE *f, const struct var_assign *var_assign, int depth) {
	space(f, depth);
	fprintf(f, "var_assign: %s = ", var_assign->var_name);
	print_words(f, var_assign->var_value);
	fprintf(f, "\n");
}

void print_statement(FILE *f, const struct statement *statement, int depth) {
	if (statement->for_loop != NULL) {
		print_for_loop(f, statement->for_loop, depth);
	}
	if (statement->conditional != NULL) {
		print_conditional(f, statement->conditional, depth);	
	}
	if (statement->program != NULL) {
		print_program(f, statement->program, depth);
	}
	if (statement->var_assign != NULL) {
		print_var_assign(f, statement->var_assign, depth);	
	}
}

void print_script(FILE *f, const struct script *script, int depth) {
	space(f, depth);
	fprintf(f, "script:\n");
	for (const struct statement *s = script->first; s != NULL; s = s->next) {
		print_statement(f, s, depth + 1);
	}
}

#define FREE_LL(t, x)	do { struct t *p = x->first; while (p) { struct t *next = p->next; free_##t(p); p = next; } } while(0)

void free_word(struct word *word) {
	free((void *) word->text);
	free(word);
}

void free_words(struct words *words) {
	FREE_LL(word, words);
	free(words);
}

void free_program(struct program *program) {
	if (program->words)
		free_words(program->words);
	if (program->lhs)
		free_program(program->lhs);
	if (program->rhs)
		free_program(program->rhs);
	free(program);
}

void free_for_loop(struct for_loop *for_loop) {
	free_word(for_loop->var_name);
	free_words(for_loop->var_values);
	free_script(for_loop->script);
	free(for_loop);
}

void free_var_assign(struct var_assign *var_assign) {
	free((void *)var_assign->var_name);
	free_words(var_assign->var_value);
	free(var_assign);
}

void free_statement(struct statement *statement) {
	if (statement->for_loop)
		free_for_loop(statement->for_loop);
	if (statement->conditional)
		free_conditional(statement->conditional);
	if (statement->program)
		free_program(statement->program);
	if (statement->var_assign)
		free_var_assign(statement->var_assign);
	free(statement);
}

void free_script(struct script *script) {
	FREE_LL(statement, script);
	free(script);
}

void free_conditional_part(struct conditional_part *conditional_part) {
	free_script(conditional_part->predicate);
	free_script(conditional_part->if_true_block);
	free(conditional_part);
}

void free_conditional(struct conditional *conditional) {
	FREE_LL(conditional_part, conditional);
	if (conditional->else_block)
		free_script(conditional->else_block);
	free(conditional);
}

static void context_empty_env_tree(struct context *context) {
	while (context->env_tree != NULL) {
		const char *e = *(const char **)context->env_tree;
		tdelete(e, &context->env_tree, env_tree_compare);
		free((void *)e);
	}
}

void free_context(struct context *context) {
	context_empty_env_tree(context);
	context_empty_pid_wait_tree(context);
	free(context);
}

static struct argv_buf *argv_buf_expand(struct argv_buf *buf) {
	buf->capacity <<= 1;
	struct argv_buf *ret = realloc(buf, sizeof(*buf) + buf->capacity);
	if (ret == NULL) {
		fprintf(stderr, "realloc() failed for argv_buf!\n");
		exit(1);
	}
	return ret;
}

// Add a single char to the argv buf, expanding if needed.
static struct argv_buf *argv_buf_putchar(struct argv_buf *buf, char c) {
	if (buf->used >= buf->capacity - 1) {
		buf = argv_buf_expand(buf);
	}
	buf->buf[buf->used++] = c;
	return buf;
}

// Add an entire string to the argv buf plus a null terminator, expanding if needed.
static struct argv_buf *argv_buf_puts(struct argv_buf *buf, const char *s) {
	for (const char *c = s; c && *c; c++)
		buf = argv_buf_putchar(buf, *c);
	buf = argv_buf_putchar(buf, 0);
	return buf;
}

struct argv_buf *make_argv(const struct context *context, const struct words *words) {
	struct argv_buf *buf = malloc(sizeof(*buf));
	buf->argv = 0;
	buf->argc = 0;
	buf->used = 0;
	buf->capacity = sizeof(buf->buf);

	for (const struct word *word = words->first; word != NULL; word = word->next) {
		if (word->is_var) {
			const char *var = context_get_var(context, word->text);
			if (var) {
				buf = argv_buf_puts(buf, var);	// FIXME
			}
		} else {
			buf = argv_buf_puts(buf, word->text);
		}
	}

	// Argv in a separate allocation, for pointer stability of the underlying string
	// data if a vector expansion occurs due to argv.
	int max_argc = 2;
	buf->argv = malloc(sizeof(char *) * (max_argc + 1));

	int in_word = 0;
	for (size_t i = 0; i < buf->used; i++) {
		char *c = &buf->buf[i];
		if (buf->argc == max_argc - 1) {
			max_argc <<= 1;
			buf->argv = realloc(buf->argv, sizeof(char *) * (max_argc + 1));
		}
		if (!in_word && !isspace(*c) && *c != 0) {
			in_word = 1;
			buf->argv[buf->argc++] = c;
		} else if (in_word && (isspace(*c) || *c == 0)) {
			in_word = 0;
			*c = 0;	// convert spaces to null termination for argv.
		} else if (!in_word) {
			*c = 0;
		}
	}
	buf->argv[buf->argc] = NULL;
	return buf;
}

void free_argv(struct argv_buf *buf) {
	if (buf->argv) free(buf->argv);
	free(buf);
}

int run_conditional(struct context *context, const struct conditional *conditional, struct run_context *run_context) {
	int rc = 0;
	for (const struct conditional_part *cp = conditional->first; cp != NULL; cp = cp->next) {
		rc = run_script(context, cp->predicate, run_context);
		if (rc == 0) {
			// take this block and return.
			return run_script(context, cp->if_true_block, run_context);
		}
	}
	if (conditional->else_block != NULL) {
		rc = run_script(context, conditional->else_block, run_context);
	}
	return rc;
}

int run_for_loop(struct context *context, const struct for_loop *for_loop, struct run_context *run_context) {
	int rc = 0;
	struct argv_buf *buf = make_argv(context, for_loop->var_values);

	for (int i = 0; i < buf->argc; i++) {
		context_set_var(context, for_loop->var_name->text, buf->argv[i]);
		rc = run_script(context, for_loop->script, run_context);
	}

	free_argv(buf);
	return rc;
}

int run_var_assign(struct context *context, const struct var_assign *var_assign, struct run_context *run_context) {
	struct argv_buf *buf = make_argv(context, var_assign->var_value);
	(void)run_context;

	context_set_var(context, var_assign->var_name, buf->argv[0]);

	free_argv(buf);
	return 0;
}

int run_fg_statement(struct context *context, const struct statement *statement, struct run_context *run_context) {
	if (statement->conditional)
		return run_conditional(context, statement->conditional, run_context);
	if (statement->program) {
		return run_program(context, statement->program, run_context);
	}
	if (statement->for_loop)
		return run_for_loop(context, statement->for_loop, run_context);
	if (statement->var_assign)
		return run_var_assign(context, statement->var_assign, run_context);
	return -ENOSYS;
}

/******************************************************************************************************
 *                                                                                                    *
 * Start of functions for you to implement. You will likely want to use other functions in this file. *
 *                                                                                                    *
 ******************************************************************************************************/


/*static*/ void context_pid_wait_tree_add(struct context *context, int pid) {
	tsearch((void*)(uintptr_t)pid, &context->pid_wait_tree, pid_wait_tree_compare);
}

/*static*/ void context_empty_pid_wait_tree(struct context *context) {
	// This loop will iterate through all pids inserted into pid_wait_tree.
	while (context->pid_wait_tree != NULL) {
		int pid = *(int *)context->pid_wait_tree;
		tdelete((void *)(uintptr_t)pid, &context->pid_wait_tree, pid_wait_tree_compare);
		// Here we want to wait on the child process with process id 'pid'.
		// Your code goes here (Section 5)
	}
}

// Determines if the given command (string) is an intrinsic command (see sections 3 and 4)
// Return 1 if the command is an intrinsic and 0 otherwise
int is_builtin(const char *argv0) {
	if (strcmp(argv0, "exit") == 0)
		return 1;

	// Your code goes here (Sections 4 & 5)

	// Detects cd as an intrinsic command
	if(strcmp(argv0, "cd") == 0)
		return 1;

	if(strcmp(argv0, "pwd") == 0)
		return 1;

	return 0;
}

// Handle an intrinsic command.
// Takes in context, instrinsic command + arguments, and the length of argv
// Hint: which system call can change the current working directory of a process?
// Hint: the home directory is in the environment variable 'HOME'
int handle_builtin(struct context *context, char **argv, int argc) {
	(void) context;	 // Line can likely be removed once implementation is done.
	(void) argv;     // Line can likely be removed once implementation is done.
	(void) argc;     // Line can likely be removed once implementation is done.
	if (strcmp(argv[0], "exit") == 0)
		exit(0);

	// Your code goes here (Sections 4 & 5)

	// Check to see if the first argument is the cd command
	if(strcmp(argv[0], "cd") == 0) {
		// If there is more than one argument (cd + another string), then we're trying to navigate
		// to another directory. Otherwise, assume we're moving into the HOME directory
	 	if(argc > 1) {
			// Execute the chdir syscall to change directories and check for an error
			if(chdir(argv[1]) == -1) {
				printf("[lsh_ast.c -> handle_builtin()] chdir error: %d\n", errno);
				return EINVAL;
			}
		} else {
			// Execute the chdir syscall to change to the $HOME directory by getting the
			// relevant environment variable and check for an error
			char *home_path = getenv("HOME");
			if(home_path == NULL) {
				printf("[lsh_ast.c -> handle_builtin()] cd getenv error\n");
				return EINVAL;
			}

			if(chdir(getenv("HOME")) == -1) {
				printf("[lsh_ast.c -> handle_builtin()] chdir error: %d\n", errno);
				return EINVAL;
			}
		}

		// Code adapted from Mic on StackOverflow to get current working directory
		char cwd[PATH_MAX+1];
		if(getcwd(cwd, sizeof(cwd)) == NULL) {
			printf("[lsh_ast.c -> handle_builtin()] getcwd error\n");
			return EINVAL;
		}

		// Set the $PWD environment variable to the current working directory and check
		// for an error
		if(setenv("PWD", cwd, 1) == -1) {
			printf("[lsh_ast.c -> handle_builtin()] setenv error: %d\n", errno);
			return EINVAL;
		}
	}

	// Check to see if the first argument is the pwd command
	if(strcmp(argv[0], "pwd") == 0) {
		// Retrieve the working directory path from the $PWD environment variable and check
		// for an error
		char *wd_path = getenv("PWD");
		if(wd_path == NULL) {
			printf("[lsh_ast.c -> handle_builtin()] pwd getenv error\n");
			return EINVAL;
		}

		// Print the working directory
		printf("%s\n", wd_path);
	}

	return EINVAL;
}

// Run one program, waiting for it to complete.
// Hints:
// - for Section 3:
//   - man 3 fork
//   - man 3 execvp
//   - man 3 waitpid
// - for Section 7:
//   - man 3 dup2
//   - What might you want to do with 'run_context' to pass things from run_pipe_program to here?
//   - How might the run_context differ for the left-hand-side program vs the right-hand-side program?
int run_one_program(struct context *context, const struct program *program, struct run_context *run_context, struct argv_buf *argv) {

	int rc = -ENOSYS;

	// Your code goes here (Section 3 & 7)

	// Fork the parent process and save the pid of the child
	pid_t child_pid = fork();

	// Execute the requested command from the program structure *program and pass in the arguments
	// from the argv_buf, *buf
	if(execvp(program->words->first->text, argv->argv) == -1)
		printf("[lsh_ast.c -> run_one_program()] execvp error: %d\n", errno);

	// Wait for the child process, identified by the pid generated by fork(), to terminate, and
	// pass its wstatus argument to the rc variable. No flags are used in this function call as
	// indicated by the third argument being zero.
	if(waitpid(child_pid, &rc, 0) != child_pid)
		printf("[lsh_ast.c -> run_one_program()] waitpid error: %d\n", errno);

	return rc;
}

// Run one or many programs.
// If the command is an intrinsic (like 'cd'), it will be handled by handle_builtin.
// Compound programs are handled by dedicated handlers, which will need modifications.
// Otherwise, you need to write code to handle it here in a child subprocess.
// Hint: The shell (parent) needs to wait for the child to finish executing.
int run_program(struct context *context, const struct program *program, struct run_context *run_context) {

	if (program->run_fn) {
		return program->run_fn(context, program, run_context);
	}
	if (program->script) {
		return run_script(context, program->script, run_context);
	}

	CHECK(program->words);

	int rc;
	struct argv_buf *argv = make_argv(context, program->words);

	// If this is a builtin, run it. Otherwise, fork and exec.
	if (is_builtin(argv->argv[0])) {
		rc = handle_builtin(context, argv->argv, argv->argc);
		goto out;
	}

	rc = run_one_program(context, program, run_context, argv);

out:
	free_argv(argv);
	return rc;
}

// Run a command in the background (spawn as a child process but do not wait)
// Note: need to keep track of all background child PIDs in case the user wants to call wait
void run_bg_statement(struct context *context, const struct statement *statement, struct run_context *run_context) {
	struct words *prog_words = context->script->first->program->words;
	const char *command = prog_words->first->text;
	const char **argv = (const char**)malloc(sizeof(char*));
	int argc = 0;
	struct word *current_word = prog_words->first->next;
	while(current_word != NULL) {
		*(argv + argc) = current_word->text;
		current_word = current_word->next;
		argc++;

		const char **new_argv = (const char**)malloc((argc+1) * sizeof(char*));
		memcpy(new_argv, argv, argc * sizeof(char*));
		argv = new_argv;
	}
	
	*(argv + argc) = (const char*)NULL;
	if(argc == 0)
		argv = NULL;

	// Fork the parent process and save the pid of the child
	pid_t child_pid = fork();

	// Execute the requested command from the
	if(execvp(command, (char* const*)argv) == -1)
		printf("[lsh_ast.c -> run_one_program()] execvp error: %d\n", errno);

	context_pid_wait_tree_add(context, child_pid);

	
	waitpid(child_pid, NULL, WNOHANG);

	free(argv);
}

// Execute the pipe stream of commands, which is two commands chained together with a pipe (|)
// i.e. cat /usr/share/dict/words | grep ^z.*o$
// See the pipe_steam struct in lsh_ast.h. It contains the command before the pipe and the command after the pipe.
// run_pipe_stream returns the status code of the last member of the pipe. 0 = success, anything else is failure.
// Hint: see 'man pipe' to create a pipe between processes
// Hint: see 'man dup2' for making one file descriptor (i.e. stdin or stdout) point to another.
int run_pipe_programs(struct context *context, const struct program *program, struct run_context *run_context) {
	int rc = -ENOSYS;

	// Your code goes here (Section 7)

	return rc;
}

// && means you run the rhs only if the lhs returns 0/success.
int run_and_programs(struct context *context, const struct program *program, struct run_context *run_context) {
	int rc = -ENOSYS;

	// Your code goes here (Section 6)

	return rc;
}

// || means you run the rhs only if the lhs returns non-zero/failure.
int run_or_programs(struct context *context, const struct program *program, struct run_context *run_context) {
	int rc = -ENOSYS;

	// Your code goes here (Section 6)

	return rc;
}

/******************************************
 *                                        *
 * End of functions for you to implement. *
 *                                        *
 ******************************************/

int run_statement(struct context *context, const struct statement *statement, struct run_context *run_context) {
	if (statement->background) {
		run_bg_statement(context, statement, run_context);
		return 0;
	} else {
		return run_fg_statement(context, statement, run_context);
	}
}

int run_script(struct context *context, const struct script *script, struct run_context *run_context) {
	int rc;
	for (const struct statement *s = script->first; s; s = s->next) {
		rc = run_statement(context, s, run_context);
	}
	return rc;
}

static const char *context_get_var_raw(const struct context *context, const char *key) {
	void *t = tfind(key, &context->env_tree, env_tree_compare);
	if (t == NULL) {
		return NULL;
	}
	const char *s = *(const char **)t;
	return s;
}

const char *context_get_var(const struct context *context, const char *key) {
	const char *s = context_get_var_raw(context, key);
	return &s[strlen(key) + 1];
}

void context_set_var(struct context *context, const char *key, const char *value) {
	value = value ? value : "";
	char *buf = malloc(strlen(key) + strlen(value) + 2);
	const char *p = key;
	char *b = buf;
	while (*p && *p != '=') *b++ = *p++;
	*b++ = '=';
	p = value;
	while (*p) *b++ = *p++;
	*b++ = 0;

	//fprintf(stderr, "%s: buf: '%s'\n", __FUNCTION__, buf);

	// Delete any existing value.
	const char *s;
	while ((s = context_get_var_raw(context, key)) != NULL) {
		tdelete(buf, &context->env_tree, env_tree_compare);
		free((void *)s);
	}
	tsearch(buf, &context->env_tree, env_tree_compare);
}