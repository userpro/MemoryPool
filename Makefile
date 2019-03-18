CC = gcc
CPP = g++
SOURCES = memorypool.c
MAIN_SOURCES = main.cpp
MAIN_OUTPUT = main
EXAMPLE_SOURCES = example.c
EXAMPLE_OUTPUT = example

run_main:
	$(CPP) $(MAIN_SOURCES) $(SOURCES) -o $(MAIN_OUTPUT).out
	./$(MAIN_OUTPUT).out

run_example:
	$(CC) $(EXAMPLE_SOURCES) $(SOURCES) -o $(EXAMPLE_OUTPUT).out
	./$(EXAMPLE_OUTPUT).out

.PHONY: clean
clean:
	rm *.out