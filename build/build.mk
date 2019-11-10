CC = gcc
FILES = ms_main.c external/src/glad.c
APP_NAME = ms
CFLAGS = -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-parameter -I external/include
LDFLAGS = -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm