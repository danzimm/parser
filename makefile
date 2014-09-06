
CXX=clang++
CFLAGS=-Wall -Werror -std=c++11 -O3

.PHONY: all clean

all: tst

clean:
	rm -rf *.o
	rm -rf tst

tst: tst.cc.o lexer.hpp
	$(CXX) $(filter %.o,$^) -o $@

%.cc.o: %.cc
	$(CXX) -c $< -o $@ $(CFLAGS)

