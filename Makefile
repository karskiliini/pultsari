CC := g++

# enable these to use ncurses UI
#CURSESFLAG := -DUSE_NCURSES
#CURSESLIB := -lncurses

CFLAGS := -Wall -g -std=c++14 ${CURSESLIB}
TARGET := pultsari
export REL_PATH := $(shell pwd)
INC :=-I$(REL_PATH)/include

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard src/*.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
%.o: %.cpp
	$(CC) $(CFLAGS) $(CURSESFLAG) -c $< ${INC}
	mv *.o src/
clean:
	rm -rf $(TARGET) *.o src/*.o

.PHONY: all clean