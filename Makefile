# Compiler and flags
CC = gcc
CFLAGS = -Wall -lm

# sample usage: make run ARGS="1024 12 64 p1.proc p2.proc p3.proc"

# Output file name
OUTPUT = paging

# Source files
SRC = paging.c

# Compile the source file
compile:
	$(CC) $(CFLAGS) -c $(SRC) -o paging.o

# Build the final executable
build: compile
	$(CC) paging.o -o $(OUTPUT) $(CFLAGS)

# Run the program with arguments passed from the command line
run: build
	./$(OUTPUT) $(ARGS)

# Clean up the compiled files
clean:
	rm -f paging.o $(OUTPUT)

.PHONY: compile build run clean