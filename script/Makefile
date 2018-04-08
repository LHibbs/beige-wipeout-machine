TARGET   = a.out
CC       = gcc
CCFLAGS  = -pedantic -Wall -Werror -fdiagnostics-color -std=gnu99
LDFLAGS  = -lm -lwiringPi
SOURCES  = $(wildcard *.c) 
INCLUDES = $(wildcard *.h)  
OBJECTS  = $(SOURCES:.c=.o)

all:$(TARGET)

$(TARGET):$(OBJECTS)
	$(CC) -o $(TARGET) $(LDFLAGS) $(OBJECTS)

$(OBJECTS):$(SOURCES) $(INCLUDES)
	$(CC) -c $(CCFLAGS) $(SOURCES) 

d:
	$(CC) -g $(CCFLAGS) $(SOURCES)

clean:
	rm -f $(TARGET) $(OBJECTS)
