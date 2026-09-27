CC = gcc
CFLAGS = -O3 -march=native -mavx2 -mfma -flto -fomit-frame-pointer -Wall -Wextra -Iinclude -D_GNU_SOURCE
LDFLAGS = -lm -flto

ASM_SRCS = src/asm/fastpath_x86_64.s
C_SRCS = src/zenith_memory.c \
         src/zenith_value.c \
         src/zenith_parser.c \
         src/zenith_builtins.c \
         src/zenith_jit.c \
         src/zenith_vm.c \
         src/main.c

OBJS = $(C_SRCS:.c=.o) $(ASM_SRCS:.s=.o)
TARGET = bin/zenithpy

all: $(TARGET)

bin:
	mkdir -p bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) -c $< -o $@

$(TARGET): bin $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)
	@echo "Build successful: $(TARGET)"

clean:
	rm -rf bin $(OBJS)

test: $(TARGET)
	@bash tests/run_tests.sh

.PHONY: all clean test
