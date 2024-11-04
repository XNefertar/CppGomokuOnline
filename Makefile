.PHONY: Game
Game:Game.cc Logger.hpp UserTable.hpp OnlineManage.hpp Room.hpp RoomManage.hpp\
	 MatchQueue.hpp Matcher.hpp Session.hpp SessionManage.hpp\
	 JsonCpp.hpp StringSplit.hpp FileRead.hpp
	g++ -g -std=c++11 $^ -o $@ -L/usr/lib64/mysql -lmysqlclient -ljsoncpp -lpthread

.PHONY:clean
clean:
	rm -rf Game
