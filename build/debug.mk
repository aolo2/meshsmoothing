include build/build.mk

BUILD_PATH = build/debug
CFLAGS += -O0 -g  -fsanitize=address -fsanitize=undefined

all:
	@mkdir -p $(BUILD_PATH)
	@/usr/bin/time -f "[TIME] [%E] Built executable $(BUILD_PATH)/$(APP_NAME)" $(CC) $(CFLAGS) $(FILES) -o $(BUILD_PATH)/$(APP_NAME) $(LDFLAGS)