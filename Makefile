CXX ?= g++
CXXSTD ?= -std=c++14
WARNINGS = -Wall -Wextra
OPTIMIZE ?= -O3 -march=native -flto=auto -fno-math-errno -DNDEBUG
ARGS ?=
# depth used by `make bench`
DEPTH ?= 10
# threads used by `make bench` / `make selfplay`
THREADS ?= 1

CXXFLAGS += $(WARNINGS) $(CXXSTD) -MMD -MP -Isrc -I/opt/homebrew/include
LDLIBS += -pthread

# on Windows every thread inherits the stack size stored in the PE header, and
# the 1 MiB default is not enough for a deep recursive search
ifeq ($(OS),Windows_NT)
  LDFLAGS += -Wl,--stack,67108864
endif

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
PERFT_BIN = $(BIN_DIR)/perft
UCI_BIN = $(BIN_DIR)/uci

APP_SRCS = $(APP_DIR)/main.cpp
TEST_SRCS = $(wildcard $(TEST_DIR)/test-*.cpp)
BENCH_SRCS = $(APP_DIR)/speed-bench.cpp
SELFPLAY_SRCS = $(APP_DIR)/selfplay.cpp
PERFT_SRCS = $(APP_DIR)/perft-main.cpp
UCI_SRCS = $(APP_DIR)/uci.cpp
CORE_SRCS = $(wildcard $(SRC_DIR)/*.cpp)

APP_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(APP_SRCS))
CORE_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CORE_SRCS))
TEST_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TEST_SRCS))
BENCH_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(BENCH_SRCS))
SELFPLAY_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SELFPLAY_SRCS))
PERFT_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(PERFT_SRCS))
UCI_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(UCI_SRCS))

DEPS = $(APP_OBJS:.o=.d) $(CORE_OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(BENCH_OBJS:.o=.d) \
       $(SELFPLAY_OBJS:.o=.d) $(PERFT_OBJS:.o=.d) $(UCI_OBJS:.o=.d)

.PHONY: all run test bench selfplay perft uci release clean help

all: $(APP_BIN)

# portable build for machines whose cpu is not known in advance
release: OPTIMIZE = -O3 -mpopcnt -flto=auto -DNDEBUG
release: clean $(APP_BIN) $(BENCH_BIN) $(UCI_BIN)

$(APP_BIN): $(APP_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(TEST_BIN): $(TEST_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BENCH_BIN): $(BENCH_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(SELFPLAY_BIN): $(SELFPLAY_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(PERFT_BIN): $(PERFT_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(UCI_BIN): $(UCI_OBJS) $(CORE_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $(LDFLAGS) $^ -o $@ $(LDLIBS)

run: $(APP_BIN)
	./$(APP_BIN) $(ARGS)

test: $(TEST_BIN)
	./$(TEST_BIN) $(ARGS)

bench: $(BENCH_BIN)
	./$(BENCH_BIN) $(if $(ARGS),$(ARGS),$(DEPTH) $(THREADS))

selfplay: $(SELFPLAY_BIN)
	./$(SELFPLAY_BIN) $(ARGS)

perft: $(PERFT_BIN)
	./$(PERFT_BIN) $(ARGS)

uci: $(UCI_BIN)
	./$(UCI_BIN) $(ARGS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(OPTIMIZE) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

help:
	@echo "make            build bin/main"
	@echo "make run        play against the engine from the console"
	@echo "make test       run the unit tests"
	@echo "make bench      search the benchmark suite, e.g. make bench DEPTH=12 THREADS=4"
	@echo "make perft      run the move generator node counts, e.g. make perft ARGS=6"
	@echo "make uci        start the UCI protocol loop"
	@echo "make selfplay   let the engine play against itself"
	@echo "make release    portable build without -march=native"

clean:
	@rm -rf $(BIN_DIR) $(BUILD_DIR) *.gcda
