.PHONY:all
all:server jsoncpp

jsoncpp:JsonCppDemo.cc
	g++ -o $@ $^ -ljsoncpp -std=c++14

server:http_server.cc
	g++ -o $@ $^ -lpthread -std=c++14

.PHONY:clean
clean:
	rm -rf server jsoncpp