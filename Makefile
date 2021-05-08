client:
	g++ client/main.cpp -o run-client

server:
	g++ server/main.cpp -o run-server

clean:
	rm -rf run-*

.PHONY: clean client server