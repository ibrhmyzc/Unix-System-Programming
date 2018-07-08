CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS =
OBJFILES = hw5.o 131044011_main.o -lm
TARGET = floristApp

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm *.o $(objects) floristApp
