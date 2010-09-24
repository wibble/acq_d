CC = gcc
CFLAGS = -std=gnu99 -p -O2 -Wall
LDFLAGS = -lcomedi -lm

#INCLUDE = -I./comedilib/include
#LIBS = -L./comedilib/lib

SOURCES = acq_d.c acq_helpers.c
OBJECTS = $(SOURCES:.c=.o)
EXEC = acq_d

all: $(SOURCES) $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

.cpp.o:
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o $(EXEC)

