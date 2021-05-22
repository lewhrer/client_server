client:
	g++ client/*.cpp  -o run-client

server:
	g++ server/*.cpp -o run-server

clean:
	rm -rf run-*

.PHONY: clean client server