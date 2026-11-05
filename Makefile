CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -pedantic
LDLIBS = -lm

SRC_DIRS = $(wildcard src/*/) $(wildcard src/eds/*/)
INCLUDES = $(addprefix -I,$(SRC_DIRS))
SRCS = $(wildcard $(addsuffix *.c,$(SRC_DIRS)))
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))

.PHONY: all clean
all: trabalho

trabalho: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

clean:
	rm -rf build trabalho

-include $(OBJS:.o=.d)
