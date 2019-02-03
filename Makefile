
default:
	g++ -Wall prosqlite.cpp -o prosqlite

run:
	./prosqlite

clean:
	rm ./prosqlite