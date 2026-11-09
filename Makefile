CC := g++

# enable these to use ncurses UI
#CURSESFLAG := -DUSE_NCURSES
#CURSESLIB := -lncurses

CFLAGS := -Wall -g -std=c++14
TARGET := pultsari
INC := -Iinclude

SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CURSESLIB)

# -MMD -MP rebuild objects whose headers changed
%.o: %.cpp
	$(CC) $(CFLAGS) $(CURSESFLAG) -MMD -MP $(INC) -c $< -o $@

clean:
	rm -f $(TARGET) src/*.o src/*.d

-include $(DEPS)

.PHONY: all clean
