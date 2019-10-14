CC = gcc
CPP = g++
GCCFLAG = -Wall -lpthread
SOURCES = memorypool.c
MAIN_SOURCES = test.cpp
MAIN_OUTPUT = test
EXAMPLE_SOURCES = example.c
EXAMPLE_OUTPUT = example
THREAD_SAFE = -D _Z_MEMORYPOOL_THREAD_

run_single_test:
	$(CPP) $(GCCFLAG) $(MAIN_SOURCES) $(SOURCES) -o $(MAIN_OUTPUT).out
	./$(MAIN_OUTPUT).out

# 多线程
run_multi_test:
	$(CPP) $(GCCFLAG) $(MAIN_SOURCES) $(SOURCES) $(THREAD_SAFE) -o $(MAIN_OUTPUT).out
	./$(MAIN_OUTPUT).out

run_example:
	$(CC)  $(GCCFLAG) $(EXAMPLE_SOURCES) $(SOURCES) -o $(EXAMPLE_OUTPUT).out
	./$(EXAMPLE_OUTPUT).out

.PHONY: clean
clean:
	rm *.out