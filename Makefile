PROG=coherence_sim
SRC_DIR=src
SRC=$(SRC_DIR)/coherence_sim.cpp
DEPS=$(SRC_DIR)/coherence_sim.h
CC=g++
CFLAGS= -g -Wall -Wextra

default : $(PROG)

$(PROG): $(SRC) $(DEPS)
	$(CC) $(CFLAGS) $(SRC) -o $(PROG)

.PHONY : clean

clean: 
	rm $(PROG)