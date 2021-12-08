texbuild: main.o
	g++ main.o -o texbuild
	cp "/rowan/Documents/Programming/C++/TeXbuild/texbuild" "/home/rowan/bin/texbuild"
main.o : main.cpp
	g++ -Wall -std=c++11 -c main.cpp
