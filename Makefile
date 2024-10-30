server:http_server.cc
	g++ -o $@ $^ -lpthread -std=c++14

.PHONY:clean
clean:
	rm -rf server