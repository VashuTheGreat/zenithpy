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

PY_CONFIG = python3-config
PY_CFLAGS = $(shell $(PY_CONFIG) --cflags 2>/dev/null)
PY_LDFLAGS = $(shell $(PY_CONFIG) --ldflags 2>/dev/null)
EXT_SUFFIX = $(shell $(PY_CONFIG) --extension-suffix 2>/dev/null)
EXTENSION = zenith_accelerator$(EXT_SUFFIX)

all: $(TARGET) extension

bin:
	mkdir -p bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) -c $< -o $@

src/asm/fastpath_pic.o: src/asm/fastpath_x86_64.s
	$(CC) -c -fPIC $< -o $@

$(TARGET): bin $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)
	@echo "Build successful: $(TARGET)"

extension: src/asm/fastpath_pic.o src/zenith_extension.c
	$(CC) -shared -fPIC $(CFLAGS) $(PY_CFLAGS) src/zenith_extension.c src/asm/fastpath_pic.o -o $(EXTENSION) $(PY_LDFLAGS)
	@echo "Extension build successful: $(EXTENSION)"

clean:
	rm -rf bin $(OBJS) src/asm/fastpath_pic.o zenith_accelerator*.so

test: $(TARGET)
	@bash tests/run_tests.sh

.PHONY: all clean test extension

