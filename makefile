bin/sugarscratch: sugarscratch.cpp
	mkdir -p bin
	g++ -o $@ -std=c++11 -Wall $<
