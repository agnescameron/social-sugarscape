bin/sugarscratch: sugarscratch.cpp
	mkdir -p bin
	g++ -o $@ -l sqlite3 -std=c++14 -Wall $<