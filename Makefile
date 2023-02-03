CC=g++

CFLAFS_DEBUG=-g3 -O1 -pg -ggdb
CFLAFS_RELEASE=-g0 -O3
CFLAGS=-Wall -Wextra -Wswitch-enum -pedantic -std=gnu++17
LFLAGS=-pthread -ldl

SRCLIB=./src/map_callbacks.cpp
OBJLIB=map_callbacks.so

SRC=./src/main.cpp ./src/process.cpp ./src/ppipe.cpp ./src/boolean.cpp ./src/shell.cpp ./src/single.cpp ./src/analyze.cpp
INC=./inc/process.hpp ./inc/ppipe.hpp ./inc/boolean.hpp ./inc/shell.hpp ./inc/single.hpp ./inc/analyze.hpp
OBJ=$(SRC:.cpp=.o)


release: $(SRC) $(INC)
	$(CC) $(CFLAGS) $(CFLAFS_RELEASE) -shared -fPIC -o $(OBJLIB) $(SRCLIB)
	./update_symbols.sh
	$(CC) $(CFLAGS) $(CFLAFS_RELEASE) -o nanoshell $(SRC) $(LFLAGS)

debug: $(SRC) $(INC)
	$(CC) $(CFLAGS) $(CFLAFS_DEBUG) -shared -fPIC -o $(OBJLIB) $(SRCLIB)
	./update_symbols.sh
	$(CC) $(CFLAGS) $(CFLAFS_DEBUG) -o nanoshell $(SRC) $(LFLAGS)

