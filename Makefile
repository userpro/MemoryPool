CC = gcc
SOURCES = memorypool.c
MAIN_SOURCES = main.c
MAIN_OUTPUT = main
EXAMPLE_SOURCES = example.c
EXAMPLE_OUTPUT = example

main:
	$(CC) $(MAIN_SOURCES) $(SOURCES) -o $(MAIN_OUTPUT)
	@echo "main ok!\n"

example:
	$(CC) $(EXAMPLE_SOURCES) $(SOURCES) -o $(EXAMPLE_OUTPUT)
	@echo "example ok!\n"

.PHONY: clean
clean:
	rm *.out