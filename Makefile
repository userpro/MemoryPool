CC = gcc
CPP = g++
GCCFLAG = -Wall -lpthread
SOURCES = memorypool.c
MAIN_SOURCES = test.cpp
MAIN_OUTPUT = test
EXAMPLE_SOURCES = example.c
EXAMPLE_OUTPUT = example

run_test:
	$(CPP) $(GCCFLAG) $(MAIN_SOURCES) $(SOURCES) -o $(MAIN_OUTPUT).out
	./$(MAIN_OUTPUT).out

run_example:
	$(CC)  $(GCCFLAG) $(EXAMPLE_SOURCES) $(SOURCES) -o $(EXAMPLE_OUTPUT).out
	./$(EXAMPLE_OUTPUT).out

.PHONY: clean
clean:
	rm *.out