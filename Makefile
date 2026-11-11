CC := g++

# enable these to use ncurses UI
#CURSESFLAG := -DUSE_NCURSES
#CURSESLIB := -lncurses

CFLAGS := -Wall -g -std=c++14
TARGET := pultsari
INC := -Iinclude

SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
DEPS := $(OBJS:.o=.d) tests/unit_tests.d

all: $(TARGET)

# the version shown in the game's corner: package.json version + git commit
PKG_VERSION := $(shell sed -n 's/.*"version": *"\([^"]*\)".*/\1/p' package.json)
GIT_REV := $(shell git describe --always --dirty 2>/dev/null || echo unknown)
VERSION_STR := v$(PKG_VERSION) \xC2\xB7 $(GIT_REV)

# rewritten only when the version changes, so nothing rebuilds needlessly
include/version_gen.hpp: FORCE
	@echo '#define GAME_VERSION "$(VERSION_STR)"' > $@.tmp
	@cmp -s $@.tmp $@ && rm -f $@.tmp || mv $@.tmp $@
src/version.o: include/version_gen.hpp
FORCE:

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CURSESLIB)

# -MMD -MP rebuild objects whose headers changed
%.o: %.cpp
	$(CC) $(CFLAGS) $(CURSESFLAG) -MMD -MP $(INC) -c $< -o $@

# unit tests link the game code without main(); the game test drives the real binary
TEST_BIN := tests/unit_tests
GAME_OBJS := $(filter-out src/entrypoint.o,$(OBJS))

$(TEST_BIN): tests/unit_tests.o $(GAME_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CURSESLIB)

tests/%.o: tests/%.cpp
	$(CC) $(CFLAGS) $(CURSESFLAG) -MMD -MP $(INC) -c $< -o $@

test: $(TARGET) $(TEST_BIN)
	./$(TEST_BIN)
	python3 tests/test_game.py

clean:
	rm -f $(TARGET) $(TEST_BIN) include/version_gen.hpp src/*.o src/*.d tests/*.o tests/*.d

-include $(DEPS)

.PHONY: all clean test FORCE
