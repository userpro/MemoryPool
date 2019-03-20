CC = gcc
CPP = g++
SOURCES = memorypool.c
MAIN_SOURCES = test.cpp
MAIN_OUTPUT = test
EXAMPLE_SOURCES = example.c
EXAMPLE_OUTPUT = example

run_test:
	$(CPP) -Wall $(MAIN_SOURCES) $(SOURCES) -o $(MAIN_OUTPUT).out
	./$(MAIN_OUTPUT).out

run_example:
	$(CC)  -Wall $(EXAMPLE_SOURCES) $(SOURCES) -o $(EXAMPLE_OUTPUT).out
	./$(EXAMPLE_OUTPUT).out

.PHONY: clean
clean:
	rm *.out