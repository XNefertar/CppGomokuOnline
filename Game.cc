#include "GameServer.hpp"

int main()
{
    LOG_MSG::Log::LogInit("log.txt");
    GameServer _server(HOST, USER, PASSWD, DBNAME, PORT);
    _server.Run(8085);
    return 0;
}