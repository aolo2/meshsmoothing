include build/build.mk

CC = gcc-10
FILES = ms_main_new.c

CFLAGS = -fanalyzer
BUILD_PATH = build/debug

all:
	@$(CC) $(CFLAGS) $(FILES) -o $(BUILD_PATH)/$(APP_NAME) $(LDFLAGS)
