
CFLAGS += -Wpedantic -pedantic-errors
CFLAGS += -Werror
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Waggregate-return
CFLAGS += -Wbad-function-cast
CFLAGS += -Wcast-align
CFLAGS += -Wno-cast-qual	# free() should accept const pointers
CFLAGS += -Wno-declaration-after-statement
CFLAGS += -Wfloat-equal
CFLAGS += -Wformat=2
CFLAGS += -Wlogical-op
CFLAGS += -Wmissing-include-dirs
CFLAGS += -Wno-missing-declarations
CFLAGS += -Wno-missing-prototypes
CFLAGS += -Wnested-externs
CFLAGS += -Wpointer-arith
#CFLAGS += -Wredundant-decls
CFLAGS += -Wsequence-point
CFLAGS += -Wshadow
CFLAGS += -Wno-strict-prototypes
CFLAGS += -Wswitch
CFLAGS += -Wundef
CFLAGS += -Wunreachable-code
CFLAGS += -Wunused-but-set-parameter
CFLAGS += -Wwrite-strings
CFLAGS += -Wno-unused-parameter

LDFLAGS += -lreadline
ifneq ("$(wildcard /home/msheahan/usr/include)", "")
	CFLAGS += -I/home/msheahan/usr/include
	LDFLAGS += -L/home/msheahan/usr/lib
	LDFLAGS += -lncursesw
endif

BINARIES += lsh
BINARIES += countargs

all: $(BINARIES) $(EXPECTED_SH)

expected:
	for script in test_section?.sh ; do bash $$script > $$(echo $$script | sed s/test/expected/ | sed s/sh$$/txt/) ; done

produced:
	for script in test_section?.sh ; do ./lsh $$script > $$(echo $$script | sed s/test/produced/ | sed s/sh$$/txt/) ; done

test_all: expected produced
	for script in test_section?.sh ; do diff -y $$(echo $$script | sed s/test/produced/ | sed s/sh$$/txt/) $$(echo $$script | sed s/test/expected/ | sed s/sh$$/txt/) && echo "script '$$script' output identical!" ; done


lsh: lsh.yacc.generated.o lsh.lex.generated.o lsh.o lsh_ast.o
	gcc -g $^ $(LDFLAGS) -o $@

countargs: countargs.o
	gcc -g $^ -o $@

%.generated.o: %.generated_c
	gcc -g -x c $< -DYYDEBUG=1 -c -o $@ -MD -MF $(@:.o=.d)

%.o: %.c Makefile
	gcc -g -x c $(CFLAGS) $< -DYYDEBUG=1 -c -o $@ -MD -MF $(@:.o=.d)

%.yacc.generated_c: %.yacc Makefile
	bison -Wconflicts-sr -Wcounterexamples --locations --language=c --defines=$$(echo $@ | sed 's/c$$/h/') -o $@ $<

%.lex.generated_c: %.lex Makefile
	flex -o $@ $<

# Force lex/yacc (flex/bison) runs before regular source compilation since we depend on generated headers.
lsh.o: lsh.lex.generated_c lsh.yacc.generated_c
lsh.yacc.generated_c: lsh.lex.generated_c

clean:
	rm -f *.o *.d *.generated[_.][chdo] project1.zip project1_starter.zip $(BINARIES) expected_section?.txt

submission_zip: project1.zip

project1.zip: FORCE
	rm -f $@ && zip $@ *.lex *.yacc *.c *.h Makefile
	rm -rf ${@F}.test && mkdir ${@F}.test/ && cd ${@F}.test && unzip ../${@F} && $(MAKE) && cd .. && rm -rf ${@F}.test/ && echo "Compilation successful from submission zip!"

project1_starter.zip: FORCE
	$(MAKE) clean
	rm -rf $@ project1/
	mkdir project1/
	cp *.c *.h *.lex *.yacc Makefile setup.sh test_*.sh project1/
	cat lsh_ast.c | perl -e '$$in_soln = 0; while (<STDIN>) { if (/ifdef SOLUTION/) { $$in_soln = 1; } elsif(/endif/) { $$in_soln = 0; } else { print unless $$in_soln; }}' > project1/lsh_ast.c
	cd project1 && make clean && make && make project1.zip && make clean
	zip -r $@ project1/


.PHONY: all clean submission_zip expected produced test_all FORCE

-include *.d

