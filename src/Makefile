PROG=coherence_sim
SRC=coherence_sim.cpp
DEPS=coherence_sim.h
CC=g++
CFLAGS= -g -Wall -Wextra

default : $(PROG)

$(PROG): $(SRC) $(DEPS)
	$(CC) $(CFLAGS) $(SRC) -o ../$(PROG)

.PHONY : clean

clean: 
	rm $(PROG)