# Pro-Wizard - portable build (GNU make; gcc, clang or MinGW)
#
#   make                  build build/prowiz and build/libprowiz.a
#   make O=dir            build into another directory
#   make test             run the unit tests
#   make clean
#
# -fwrapv: the original engine relies on 32-bit signed arithmetic wrapping
# around; make that defined behaviour instead of leaving it to the optimiser.

CC      ?= cc
AR      ?= ar
CFLAGS  ?= -O2
O       ?= build

PW_CFLAGS = -std=c99 -Wall -Wextra -Wno-unused-parameter -fwrapv -Iinclude -Isrc

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

LIB_SRCS := $(wildcard src/*.c) $(wildcard src/formats/*.c)
LIB_OBJS := $(patsubst %.c,$(O)/%.o,$(LIB_SRCS))
CLI_OBJS := $(O)/cli/main.o
DEPS     := $(LIB_OBJS:.o=.d) $(CLI_OBJS:.o=.d) $(O)/tests/unit/unit_tests.d

PROWIZ   := $(O)/prowiz$(EXE)
LIBPW    := $(O)/libprowiz.a
UNITTEST := $(O)/unit_tests$(EXE)

.PHONY: all clean test

all: $(PROWIZ) $(LIBPW)

$(LIBPW): $(LIB_OBJS)
	$(AR) rcs $@ $^

$(PROWIZ): $(CLI_OBJS) $(LIBPW)
	$(CC) $(LDFLAGS) -o $@ $(CLI_OBJS) $(LIBPW)

$(O)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(PW_CFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(UNITTEST): $(O)/tests/unit/unit_tests.o $(LIBPW)
	$(CC) $(LDFLAGS) -o $@ $< $(LIBPW)

test: $(UNITTEST) $(PROWIZ)
	$(UNITTEST)

clean:
	rm -rf $(O)

-include $(DEPS)
