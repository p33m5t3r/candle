# assumes everything (incl. headers) lives in `./src`
# compiles objects to ./obj, pushes executables to ./bin

CC 		= gcc
CFLAGS_DEV 	= -Wall -Wextra -Wpedantic -Werror -O0 -g -std=c11
CFLAGS_RELEASE 	= -O3 -march=native -Wall -Wextra -std=c11

TARGET 	 = main
SRC_DIR  = ./src
OBJ_DIR  = ./obj
BIN_DIR  = ./bin
TEST_DIR = ./tests
OUT 	 = $(BIN_DIR)/$(TARGET)
SRCS 	 = $(wildcard $(SRC_DIR)/*.c)
HEADERS  = $(wildcard $(SRC_DIR)/*.h)
OBJS 	 = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_SRC = $(TEST_DIR)/test.c
TEST_OUT = $(BIN_DIR)/test

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

# test target
test: CFLAGS = $(CFLAGS_DEV)
test: $(OBJS)
	$(CC) $(CFLAGS) -o $(TEST_OUT) $(TEST_SRC) $(filter-out $(OBJ_DIR)/main.o, $(OBJS))
	$(TEST_OUT)
	valgrind --leak-check=full $(TEST_OUT)

clean:
	rm -rf $(OBJ_DIR)/*.o $(BIN_DIR)/*

.PHONY: all dev release clean
