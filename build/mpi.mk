include build/build.mk

CC = mpicc
BUILD_PATH = build/mpi

FILES = ms_mpi_main.c
CFLAGS += -DMP -O0 -g -Wno-unused-function # -fsanitize=address

all:
	@mkdir -p $(BUILD_PATH)
	@/usr/bin/time -f "[TIME] [%E] Built executable $(BUILD_PATH)/$(APP_NAME)" $(CC) $(CFLAGS) $(FILES) -o $(BUILD_PATH)/$(APP_NAME) $(LDFLAGS)