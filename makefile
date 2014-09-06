
CXX=clang++
CFLAGS=-Wall -Werror -std=c++11 -O3

.PHONY: all clean

all: tst

clean:
	rm -rf *.o
	rm -rf tst

tst: tst.cc.lexer.o
	$(CXX) $(filter %.o,$^) -o $@

%.cc.lexer.o: %.cc lexer.hpp
	$(CXX) -c $< -o $@ $(CFLAGS)

