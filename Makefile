CXXFLAGS += -Wall -Wextra -std=c++14 -MMD -MP

main: main.o
	g++ $^ -o $@

main.o: main.cpp
	g++ $(CXXFLAGS) -c $< -o $@

-include main.d

run: main
	./main

test: tests
	./tests

tests: test-main.o test-position.o
	g++ $^ -o $@

test_main.o: test-main.cpp
	g++ $(CXXFLAGS) -c $< -o $@

test_position.o: test-position.cpp
	g++ $(CXXFLAGS) -c $< -o $@

-include test-main.d test-position.d
