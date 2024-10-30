.PHONY:all
all:server jsoncpp mysql

mysql:MySQLDemo.cc
	g++ -o $@ $^ -lmysqlclient -std=c++14

jsoncpp:JsonCppDemo.cc
	g++ -o $@ $^ -ljsoncpp -std=c++14

server:http_server.cc
	g++ -o $@ $^ -lpthread -std=c++14

.PHONY:clean
clean:
	rm -rf server jsoncpp mysql