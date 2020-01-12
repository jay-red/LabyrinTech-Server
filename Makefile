LabyrinTech: server.cpp
	g++ -flto -O3 -lpthread -Wconversion -std=c++17 -Isrc -IuSockets/src server.cpp -o LabyrinTech uSockets/*.o -lz

clean:
	rm -rf LabyrinTech
