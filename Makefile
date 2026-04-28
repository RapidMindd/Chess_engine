CXX = g++
CXXFLAGS += -Wall -Wextra -std=c++14 -MMD -MP -I/opt/homebrew/include
OPTIMIZE = -O3 -march=native -flto=auto

APP_SRCS = main.cpp
CORE_SRCS = position.cpp move.cpp move_generator.cpp evaluator.cpp engine.cpp piece.cpp transposition_table.cpp zobrist.cpp
TEST_SRCS = test-main.cpp test-position.cpp test-move_generator.cpp test-engine.cpp
BENCH_SRCS = speed-bench.cpp
ITSELF_PLAY_SRCS = itself_play.cpp

APP_OBJS = $(APP_SRCS:.cpp=.o)
CORE_OBJS = $(CORE_SRCS:.cpp=.o)
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
BENCH_OBJS = $(BENCH_SRCS:.cpp=.o)
ITSELF_PLAY_OBJS = $(ITSELF_PLAY_SRCS:.cpp=.o)

DEPS = $(APP_OBJS:.o=.d) $(CORE_OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(BENCH_OBJS:.o=.d) $(ITSELF_PLAY_OBJS:.o=.d)

main: $(APP_OBJS) $(CORE_OBJS)
	$(CXX) $(OPTIMIZE) $^ -o $@
	@rm -f $(APP_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(BENCH_OBJS) $(DEPS)

run: main
	./main

test: tests
	./tests

tests: $(TEST_OBJS) $(CORE_OBJS)
	$(CXX) $(OPTIMIZE) $^ -o $@
	@rm -f $(APP_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(DEPS)

benchs: $(BENCH_OBJS) $(CORE_OBJS)
	$(CXX) $(OPTIMIZE) $^ -o $@
	@rm -f $(APP_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(BENCH_OBJS) $(DEPS)

bench: benchs
	./benchs

itselfs: $(ITSELF_PLAY_SRCS) $(CORE_OBJS)
	$(CXX) $(OPTIMIZE) $^ -o $@
	@rm -f $(APP_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(ITSELF_PLAY_OBJS) $(DEPS)

itself: itselfs
	./itselfs

%.o: %.cpp
	$(CXX) $(OPTIMIZE) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@rm -f main tests benchs itselfs $(APP_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(BENCH_OBJS) $(DEPS) *.gcda
