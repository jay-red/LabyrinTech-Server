LabyrinTech: server.o
	g++ -g -flto -O3 -lpthread -Wconversion -std=c++17 -Isrc -IuSockets/src server.o -o LabyrinTech uSockets/*.o -lz

server.o: server.cpp server.h
	g++ -std=c++17 -Isrc -IuSockets/src -c -g server.cpp

clean:
	rm -rf LabyrinTech
