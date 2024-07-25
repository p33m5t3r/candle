# assumes everything (incl. headers) lives in `./src`
# compiles objects to ./obj, pushes executables to ./bin

CC 		= gcc
CFLAGS_DEV 	= -Wall -Wextra -Wpedantic -Werror -O0 -g -std=c11
CFLAGS_RELEASE 	= -O3 -march=native -Wall -Wextra -std=c11

TARGET 	= main
SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin
OUT 	= $(BIN_DIR)/$(TARGET)
SRCS 	= $(wildcard $(SRC_DIR)/*.c)
HEADERS = $(wildcard $(SRC_DIR)/*.h)
OBJS 	= $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: dev

# dev build
dev: CFLAGS = $(CFLAGS_DEV)
dev: $(OUT)

# release build
release: CFLAGS = $(CFLAGS_RELEASE)
release: $(OUT)

# link objects 
$(OUT): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# build objects
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/*

.PHONY: all dev release clean
