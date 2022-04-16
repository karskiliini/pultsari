CC := g++
CFLAGS := -Wall -g -std=c++14 -static -static-libgcc -static-libstdc++
TARGET := pultsari
export REL_PATH := $(shell pwd)
INC :=-I$(REL_PATH)/include

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard src/*.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) ${CFLAGS} -o $@ $^ -lglut
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< ${INC}
	mv *.o src/
clean:
	rm -rf $(TARGET) *.o src/*.o

.PHONY: all clean