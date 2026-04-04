CXX = g++
CXXFLAGS += -Wall -Wextra -std=c++14 -MMD -MP -I/opt/homebrew/include

APP_SRCS = main.cpp
CORE_SRCS = position.cpp move.cpp move_generator.cpp
TEST_SRCS = test-main.cpp test-position.cpp test-move_generator.cpp

APP_OBJS = $(APP_SRCS:.cpp=.o)
CORE_OBJS = $(CORE_SRCS:.cpp=.o)
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

DEPS = $(APP_OBJS:.o=.d) $(CORE_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

main: $(APP_OBJS) $(CORE_OBJS)
	$(CXX) $^ -o $@
	@rm -f $(APP_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(DEPS)

run: main
	./main

test: tests
	./tests

tests: $(TEST_OBJS) $(CORE_OBJS)
	$(CXX) $^ -o $@
	@rm -f $(APP_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@rm -f main tests $(APP_OBJS) $(CORE_OBJS) $(TEST_OBJS) $(DEPS)
