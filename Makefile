CC = gcc
PREFIX = /usr/local

TARGET = shaderboy
C_SRC = shaderboy.c

OBJS = $(C_SRC:.c=.o)
DEPS = $(OBJS:.o=.d)

LIBS = -lSDL -lGL -lm
LFLAGS = -L/usr/local/lib -L/usr/lib

CFLAGS = -std=c99 -D_POSIX_C_SOURCE
CFLAGS += -I. -I/usr/include/
CFLAGS += -g -O2
CFLAGS += -Werror
CFLAGS += -Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow -Wpointer-arith -Wcast-qual -Wno-missing-braces -Wno-unused-parameter -Wuninitialized

#DEBUG=DEBUG
ifdef DEBUG
	CFLAGS += -fstack-protector -fsanitize=address -ggdb3 -Og
	LIB += -lasan
endif

-include $(DEPS)
%.d : %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

# Targets
.PHONY: all
all: tags $(TARGET)

.PHONY: clean
clean:
	-rm -f $(OBJS) $(DEPS) $(TARGET) tags

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -g -o $(TARGET) $(OBJS) $(LIBS)

tags: $(C_SRC)
	-ctags -R

.DEFAULT_GOAL := all
