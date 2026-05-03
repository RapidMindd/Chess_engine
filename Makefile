CXX = g++
CXXFLAGS += -Wall -Wextra -std=c++14 -MMD -MP -Isrc -I/opt/homebrew/include
OPTIMIZE = -O3 -march=native -flto=auto
ARGS ?=

SRC_DIR = src
APP_DIR = apps
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = bin

APP_BIN = $(BIN_DIR)/main
TEST_BIN = $(BIN_DIR)/tests
BENCH_BIN = $(BIN_DIR)/benchmark
SELFPLAY_BIN = $(BIN_DIR)/selfplay

APP_SRCS = $(APP_DIR)/main.cpp
TEST_SRCS = $(wildcard $(TEST_DIR)/test-*.cpp)
BENCH_SRCS = $(APP_DIR)/speed-bench.cpp
SELFPLAY_SRCS = $(APP_DIR)/selfplay.cpp
CORE_SRCS = $(wildcard $(SRC_DIR)/*.cpp)

APP_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(APP_SRCS))
CORE_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CORE_SRCS))
TEST_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TEST_SRCS))
BENCH_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(BENCH_SRCS))
SELFPLAY_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SELFPLAY_SRCS))

DEPS = $(APP_OBJS:.o=.d) $(CORE_OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(BENCH_OBJS:.o=.d) $(SELFPLAY_OBJS:.o=.d)

.PHONY: all run test bench selfplay clean

all: $(APP_BIN)

$(APP_BIN): $(APP_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $^ -o $@

run: $(APP_BIN)
	./$(APP_BIN) $(ARGS)

test: $(TEST_BIN)
	./$(TEST_BIN) $(ARGS)

$(TEST_BIN): $(TEST_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $^ -o $@

$(BENCH_BIN): $(BENCH_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $^ -o $@

bench: $(BENCH_BIN)
	./$(BENCH_BIN) $(ARGS)

$(SELFPLAY_BIN): $(SELFPLAY_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $^ -o $@

selfplay: $(SELFPLAY_BIN)
	./$(SELFPLAY_BIN) $(ARGS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@rm -rf $(BIN_DIR) $(BUILD_DIR) *.gcda
