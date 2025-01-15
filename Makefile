CC = gcc
FLAGS = -c -g
LIBS = -pthread

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
INCLUDE_DIR = include
TESTS_DIR = tests

SOURCE = $(SRC_DIR)/jobCommander.c $(SRC_DIR)/jobExecutorServer.c $(SRC_DIR)/queue.c
TESTS_SOURCE = $(TESTS_DIR)/progDelay.c

OBJS_COMMANDER = $(BUILD_DIR)/jobCommander.o $(BUILD_DIR)/queue.o
OBJS_EXECUTOR = $(BUILD_DIR)/jobExecutorServer.o $(BUILD_DIR)/queue.o
OBJS_PROG_DELAY = $(BUILD_DIR)/progDelay.o

HEADER = $(INCLUDE_DIR)/myheaders.h $(INCLUDE_DIR)/queue.h

OUT_COMMANDER = $(BIN_DIR)/jobCommander
OUT_EXECUTOR = $(BIN_DIR)/jobExecutorServer
OUT_PROG_DELAY = $(BIN_DIR)/progDelay

all: $(OUT_COMMANDER) $(OUT_EXECUTOR) $(OUT_PROG_DELAY)

$(OUT_COMMANDER): $(OBJS_COMMANDER)
	$(CC) $(OBJS_COMMANDER) -o $(OUT_COMMANDER) $(LIBS)

$(OUT_EXECUTOR): $(OBJS_EXECUTOR)
	$(CC) $(OBJS_EXECUTOR) -o $(OUT_EXECUTOR) $(LIBS)

$(OUT_PROG_DELAY): $(OBJS_PROG_DELAY)
	$(CC) $(OBJS_PROG_DELAY) -o $(OUT_PROG_DELAY) $(LIBS)

$(BUILD_DIR)/jobCommander.o: $(SRC_DIR)/jobCommander.c $(HEADER)
	$(CC) $(FLAGS) $(SRC_DIR)/jobCommander.c -o $(BUILD_DIR)/jobCommander.o

$(BUILD_DIR)/jobExecutorServer.o: $(SRC_DIR)/jobExecutorServer.c $(HEADER)
	$(CC) $(FLAGS) $(SRC_DIR)/jobExecutorServer.c -o $(BUILD_DIR)/jobExecutorServer.o

$(BUILD_DIR)/queue.o: $(SRC_DIR)/queue.c $(HEADER)
	$(CC) $(FLAGS) $(SRC_DIR)/queue.c -o $(BUILD_DIR)/queue.o

$(BUILD_DIR)/progDelay.o: $(TESTS_DIR)/progDelay.c $(HEADER)
	$(CC) $(FLAGS) $(TESTS_DIR)/progDelay.c -o $(BUILD_DIR)/progDelay.o

clean:
	rm -f $(BUILD_DIR)/*.o $(OUT_COMMANDER) $(OUT_EXECUTOR) $(OUT_PROG_DELAY)

rebuild: clean all
