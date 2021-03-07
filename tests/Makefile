SRC = ..
INCLUDES=-I./include -I$(SRC)/include
CC = gcc
LIBS = 
CFLAGS = \
	-g \
	-fprofile-arcs \
	-ftest-coverage \
	-DFUNC_ATTR=\
	-Wall 

OBJECTS = ringbuffer.o 

all: test_ringbuffer


ringbuffer.o: $(SRC)/ringbuffer.c $(SRC)/include/ringbuffer.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $< $(LIBS)



test_ringbuffer: test_ringbuffer.c ringbuffer.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)


.PHONY: clean 
clean:
	-rm -f $(OBJECTS)
	-rm test_ringbuffer
	-rm *.gcov *.gcda *.gcno


.PHONY: test
test: all
	./test_ringbuffer


.PHONY: coverage
coverage: test
	gcov -o. $(SRC)/ringbuffer.c

