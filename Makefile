CC := gcc
CFLAGS := -Wall -std=c17

PARS=parser
LEX=lexer
INC=./include
OBJ=./obj

bin/tpcas: $(OBJ)/$(PARS).tab.o $(OBJ)/lex.yy.o $(OBJ)/tree.o
	$(CC) $(CFLAGS) $^ -o $@ -lfl

#objects

$(OBJ)/%.o: src/%.c
	$(CC) $(CFLAGS) -I$(INC) -c $< -o $@

$(OBJ)/lex.yy.o: src/lex.yy.c $(INC)/$(PARS).tab.h

$(OBJ)/$(PARS).tab.o: src/$(PARS).tab.c $(INC)/tree.h

#c scripts of lex and bison

src/$(PARS).tab.c include/$(PARS).tab.h: src/$(PARS).y
	bison -d -o src/$(PARS).tab.c $<
	mv ./src/$(PARS).tab.h ./include/

src/lex.yy.c: src/$(LEX).lex
	flex $<
	mv lex.yy.c ./src/