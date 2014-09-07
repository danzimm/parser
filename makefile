
CXX=clang++
CFLAGS=-Wall -Werror -std=c++1y -O3

.PHONY: all clean

all: tst

clean:
	rm -rf *.o
	rm -rf tst

tst: tst.cc.lexer.o
	$(CXX) $(filter %.o,$^) -o $@ $(DEBUG_FLAGS)

%.cc.lexer.o: %.cc lexer.hpp
	$(CXX) -c $< -o $@ $(CFLAGS) $(DEBUG_FLAGS)

