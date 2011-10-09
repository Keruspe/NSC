all: client server

client: src/client.c src/util.c
	gcc -Wall -Wextra -std=gnu99 -pedantic -O3 -march=native -o $@ $^ -Wl,--as-needed -Wl,-O3

server: src/server.c src/util.c
	gcc -Wall -Wextra -std=gnu99 -pedantic -O3 -march=native -o $@ $^ -pthread -Wl,--as-needed -Wl,-O3

clean:
	rm -f server client
