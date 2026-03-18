CXXFLAGS += -Wall -Wextra -std=c++14 -MMD -MP

main: main.o
	g++ $^ -o $@

main.o: main.cpp
	g++ $(CXXFLAGS) -c $< -o $@

-include main.d
