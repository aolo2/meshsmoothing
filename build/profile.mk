include build/build.mk

MODE = D
CC = gcc
BUILD_PATH = build/profile

CFLAGS += -O2 -g -Wall -Wextra -Werror -march=native -DPROFILE -DTRACY_ENABLE -DTRACY_NO_EXIT -I./lib
LDFLAGS += -lX11 -lXext -lm -ldl -lpthread

CPP = g++
TRACY_NAME = tracy
TRACY_FILE = external/tracy/TracyClient.cpp

FILES += $(BUILD_PATH)/$(TRACY_NAME).so

all:
	@mkdir -p $(BUILD_PATH)
	@/usr/bin/time -f "[TIME] [%E] Built object $(BUILD_PATH)/$(TRACY_NAME).o" $(CPP) -c -fPIC $(TRACY_FILE) -o $(BUILD_PATH)/$(TRACY_NAME).o -DTRACY_ENABLE -Wno-enum-compare
	@/usr/bin/time -f "[TIME] [%E] Build shared library $(BUILD_PATH)/$(TRACY_NAME).so" $(CPP) -shared -o $(BUILD_PATH)/$(TRACY_NAME).so $(BUILD_PATH)/$(TRACY_NAME).o
	@/usr/bin/time -f "[TIME] [%E] Built executable $(BUILD_PATH)/$(APP_NAME)" $(CC) $(CFLAGS) $(FILES) -o $(BUILD_PATH)/$(APP_NAME) $(LDFLAGS)

fast:
	@mkdir -p $(BUILD_PATH)
	@/usr/bin/time -f "[TIME] [%E] Built executable $(BUILD_PATH)/$(APP_NAME)" $(CC) $(CFLAGS) $(FILES) -o $(BUILD_PATH)/$(APP_NAME) $(LDFLAGS)
