CC = gcc
CPP = g++
LEX = flex
YACC = yacc
CFLAGS += -pedantic -Wall -Wextra
CXXFLAGS += -g -Wall -Wextra
LDLIBS += -lfl -ly
YFLAGS += -d

VPATH = src


TESTS = symbol-test
EXECS = lexer parser
SRCS = y.tab.c lex.yy.c src/lexer/lexer.c src/utilities/utilities.c \
src/symbol/symbol.c src/symbol/symbol-utils.c test/symbol/symbol-test.c


all : $(EXECS)

clean :
	rm -f $(TESTS) $(EXECS) *.o lex.yy.c y.tab.c y.tab.h

# autmatically pull in dependencies on included header files
# copied from http://stackoverflow.com/a/2394668/1424966
.depend: $(SRCS)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ >>./.depend
include .depend

utilities.o :
	$(CC) -c src/utilities/utilities.c

lex.yy.o :
	$(CC) -c lex.yy.c

lex.yy.c : lexer/lexer.lex
	$(LEX) $(LFLAGS) -o $@ $<

lexer.o : lex.yy.o
	$(CC) -c src/lexer/lexer.c lex.yy.c

lexer : lexer.o
	$(CC) lexer.o lex.yy.o -o $@

y.tab.c : src/parser/parser.y lex.yy.c
	$(YACC) $(YFLAGS) -o $@ $<

y.tab.o :
	$(CC) -c y.tab.c

parser : y.tab.o utilities.o traverse.o symbol-utils.o
	$(CC) y.tab.o utilities.o traverse.o symbol-utils.o -o $@

symbol-utils.o : src/symbol/symbol-utils.c
	$(CC) -c src/symbol/symbol-utils.c

traverse.o : src/symbol/traverse.c
	$(CC) -c src/symbol/traverse.c


# tests
symbol-test.o : test/symbol/symbol-test.c
	$(CC) -c test/symbol/symbol-test.c

symbol-test : symbol-utils.o symbol-test.o utilities.o
	$(CC) symbol-utils.o symbol-test.o utilities.o -o $@
	./symbol-test

test-symbol-output : test/symbol/test-symbol-output parser
	test/symbol/test-symbol-output

