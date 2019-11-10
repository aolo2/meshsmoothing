CC = gcc
FILES = ms_main.c external/src/glad.c
APP_NAME = ms
CFLAGS = -Wall -Wextra -Werror -Iexternal/include -Wno-unused-function
LDFLAGS = -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm