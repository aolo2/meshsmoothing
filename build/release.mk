include build/build.mk

CC = gcc
BUILD_PATH = build/release

CFLAGS += -march=native -O2 -g -DNDEBUG

all:
	@mkdir -p $(BUILD_PATH)
	@/usr/bin/time -f "[TIME] [%E] Built executable $(BUILD_PATH)/$(APP_NAME)" $(CC) $(CFLAGS) $(FILES) -o $(BUILD_PATH)/$(APP_NAME) $(LDFLAGS)
