#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> webserver;
typedef webserver::message_ptr message_ptr;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

void open_handler(webserver *s, websocketpp::connection_hdl hdl)
{
    std::cout << "open_handler called with hdl: " << hdl.lock().get() << std::endl;
}

void close_handler(webserver *s, websocketpp::connection_hdl hdl)
{
    std::cout << "close_handler called with hdl: " << hdl.lock().get() << std::endl;
}

void on_message(webserver *s, websocketpp::connection_hdl hdl, message_ptr msg)
{
    std::cout << "on_message called with hdl: " << hdl.lock().get() << " and message: " << msg->get_payload() << std::endl;
    s->send(hdl, msg->get_payload(), msg->get_opcode());
}

void on_fail(webserver *s, websocketpp::connection_hdl hdl)
{
    std::cout << "on_fail called with hdl: " << hdl.lock().get() << std::endl;
}

void on_http(webserver *s, websocketpp::connection_hdl hdl)
{
    std::cout << "handler http" << std::endl;
    websocketpp::server<websocketpp::config::asio>::connection_ptr con = s->get_con_from_hdl(hdl);
    std::stringstream response;
    // 构建html响应
    response << "<!doctype html>"
             << "<html><head><title>Websocketpp Server</title></head>"
             << "<body><h1>Websocketpp Server"
             << "</h1></body></html>";
    con->set_body(response.str());
    con->set_status(websocketpp::http::status_code::ok);
}

int main(int argc, char *argv[])
{
    webserver server;
    server.set_access_channels(websocketpp::log::alevel::none);
    // server.clear_access_channels(websocketpp::log::alevel::all);
    server.init_asio();
    server.set_http_handler(bind(&on_http, &server, ::_1));

    server.set_open_handler(bind(&open_handler, &server, ::_1));
    server.set_close_handler(bind(&close_handler, &server, ::_1));
    server.set_message_handler(bind(&on_message, &server, ::_1, ::_2));
    // server.set_fail_handler(bind(&on_fail, &server, ::_1));
    server.listen(8888);
    server.start_accept();
    server.run();

    return 0;
}