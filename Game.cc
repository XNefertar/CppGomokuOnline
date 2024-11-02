#include "GameServer.hpp"

int main()
{
    // LOG_MSG::Log::LogInit("log.txt");
    GameServer _server(HOST, USER, PASSWD, DBNAME, PORT);
    INF_LOG("GameServer init success");
    std::cout << "HOST: " << HOST << std::endl;
    std::cout << "PORT: " << PORT << std::endl;
    std::cout << "USER: " << USER << std::endl;
    std::cout << "PASSWD: " << PASSWD << std::endl;
    std::cout << "DBNAME: " << DBNAME << std::endl;
    // _server.Init();
    _server.Run(8085);
    return 0;
}