CC=g++

client: client.cpp helpers.hpp requests.hpp buffer.h
	$(CC) -o client client.cpp -Wall

run: client
	./client

clean:
	rm -f *.o client
