CC=g++

client: client.cpp buffer.cpp
	$(CC) -o client client.cpp buffer.cpp -Wall

run: client
	./client

clean:
	rm -f *.o client
