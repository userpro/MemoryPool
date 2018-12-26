CC = gcc
CPP = g++
SOURCES = memorypool.c
MAIN_SOURCES = main.cpp
MAIN_OUTPUT = main
EXAMPLE_SOURCES = example.c
EXAMPLE_OUTPUT = example

main:
	$(CPP) $(MAIN_SOURCES) $(SOURCES) -o $(MAIN_OUTPUT).out
	@echo "complie main ok!\n"

example:
	$(CC) $(EXAMPLE_SOURCES) $(SOURCES) -o $(EXAMPLE_OUTPUT).out
	@echo "compile example ok!\n"

run_main:
	./$(MAIN_OUTPUT).out

run_example:
	./$(EXAMPLE_OUTPUT).out

.PHONY: clean
clean:
	rm *.out