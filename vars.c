#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;
	char buf[1024];
	printf("in %s\n", getcwd(buf, sizeof(buf)-1));
	printf("HOME is in: %s\n", getenv("HOME"));
	chdir(getenv("HOME"));
	printf("in %s\n", getcwd(buf, sizeof(buf)-1));
	return 0;
}
