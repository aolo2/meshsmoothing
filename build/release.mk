include build/build.mk

CC = gcc
FILES = ms_main.c external/src/glad.c
BUILD_PATH = build/release

CFLAGS += -march=native -O2

all:
	@mkdir -p $(BUILD_PATH)
	@/usr/bin/time -f "[TIME] [%E] Built executable $(BUILD_PATH)/$(APP_NAME)" $(CC) $(CFLAGS) $(FILES) -o $(BUILD_PATH)/$(APP_NAME) $(LDFLAGS)