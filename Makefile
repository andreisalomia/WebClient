CC = g++
CFLAGS = -Wall -Wextra -std=c++11

SRC = buffer.cpp buffer.hpp client.cpp helpers.cpp helpers.hpp requests.cpp requests.hpp
OBJ = buffer.o client.o helpers.o requests.o

TARGET = client

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.cpp %.hpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
