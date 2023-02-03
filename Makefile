CC=g++
CFLAGS=-Wall -Wextra -Wswitch-enum -pedantic -pg -ggdb -std=gnu++17
LFLAGS=-pthread -ldl
SRCLIB=map_callbacks.cpp
OBJLIB=$(SRCLIB:.cpp=.so)
SRC=main.cpp process.cpp ppipe.cpp boolean.cpp shell.cpp single.cpp analyze.cpp
INC=process.hpp ppipe.hpp boolean.hpp shell.hpp single.hpp analyze.hpp
OBJ=$(SRC:.cpp=.o)

target: $(SRC) $(INC)
	$(CC) $(CFLAGS) -shared -fPIC -o $(OBJLIB) $(SRCLIB)
	./update_symbols.sh
	$(CC) $(CFLAGS) -o main $(SRC) $(LFLAGS)
	#./main
