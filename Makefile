CC = gcc
CFLAGS  = -g -Wall
OBJECTS = task1 task2 task3
all: $(OBJECTS)


task1: task1-29255554.c
	$(CC) ${CFLAGS} -o task1 task1-29255554.c

task2: task2-29255554.c
	$(CC) ${CFLAGS} -o task2 task2-29255554.c

task3: task3-29255554.c
	$(CC) ${CFLAGS} -o task3 task3-29255554.c