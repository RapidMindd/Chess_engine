CXX = g++
CXXFLAGS += -Wall -Wextra -std=c++14 -MMD -MP -I/opt/homebrew/include
OPTIMIZE = -O3 -march=native -flto=auto
ARGS ?=

APP_BIN = main
TEST_BIN = tests
BENCH_BIN = benchmark
SELFPLAY_BIN = selfplay-runner

APP_SRCS = main.cpp
TEST_SRCS = $(wildcard test-*.cpp)
BENCH_SRCS = speed-bench.cpp
ITSELF_PLAY_SRCS = selfplay.cpp
CORE_SRCS = $(filter-out $(APP_SRCS) $(TEST_SRCS) $(BENCH_SRCS) $(ITSELF_PLAY_SRCS),$(wildcard *.cpp))

APP_OBJS = $(APP_SRCS:.cpp=.o)
CORE_OBJS = $(CORE_SRCS:.cpp=.o)
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
BENCH_OBJS = $(BENCH_SRCS:.cpp=.o)
ITSELF_PLAY_OBJS = $(ITSELF_PLAY_SRCS:.cpp=.o)

DEPS = $(APP_OBJS:.o=.d) $(CORE_OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(BENCH_OBJS:.o=.d) $(ITSELF_PLAY_OBJS:.o=.d)

.PHONY: all run test bench selfplay clean

all: $(APP_BIN)

$(APP_BIN): $(APP_OBJS) $(CORE_OBJS)
	$(CXX) $(OPTIMIZE) $^ -o $@

run: $(APP_BIN)
	./$(APP_BIN) $(ARGS)

test: $(TEST_BIN)
	./$(TEST_BIN) $(ARGS)

$(TEST_BIN): $(TEST_OBJS) $(CORE_OBJS)
	$(CXX) $(OPTIMIZE) $^ -o $@

$(BENCH_BIN): $(BENCH_OBJS) $(CORE_OBJS)
	$(CXX) $(OPTIMIZE) $^ -o $@

bench: $(BENCH_BIN)
	./$(BENCH_BIN) $(ARGS)

$(SELFPLAY_BIN): $(ITSELF_PLAY_OBJS) $(CORE_OBJS)
	$(CXX) $(OPTIMIZE) $^ -o $@

selfplay: $(SELFPLAY_BIN)
	./$(SELFPLAY_BIN) $(ARGS)

%.o: %.cpp
	$(CXX) $(OPTIMIZE) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@rm -f $(APP_BIN) $(TEST_BIN) $(BENCH_BIN) $(SELFPLAY_BIN) $(APP_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(BENCH_OBJS) $(ITSELF_PLAY_OBJS) $(DEPS) *.gcda
