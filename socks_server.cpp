#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "sock4.h"
#include "parse.h"

using namespace std;
using namespace boost::asio;
using namespace ip;

io_service global_io_service;

class ClientSession;

class WebSession : public std::enable_shared_from_this<WebSession> {
private:
    tcp::resolver _resolv{global_io_service};
    tcp::socket _socket{global_io_service};
    tcp::resolver::query _query;
    std::string req;
    std::array<char, 4096> _cmd_buffer;
    std::string host_ip;
    std::string host_port;
    enum { max_length = 4096 };
    std::array<char, 4096> bytes;
    tcp::socket _client_socket;
    std::shared_ptr<ClientSession> es_ptr;

public:
    // constructor
    WebSession(std::string host_ip, std::string host_port,
               std::shared_ptr<ClientSession> es,
               tcp::socket client_socket, std::string req)
            : _query{host_ip, host_port},
              es_ptr(es),
              _client_socket(move(client_socket)),
              req(req)

    {
    }
    void start() { do_resolve(); }
private:
    void do_client_send(std::string data){
        _client_socket.async_send(
                buffer(data),
                [this](boost::system::error_code ec, size_t /* length */) {
                    if (!ec){
//                        _socket.close();
//                        _client_socket.close();
                         do_web_read();
                    }
                    else{
                        _socket.close();
                        _client_socket.close();
                    }
                });
    }

    void do_web_send(std::string data){
        _socket.async_send(
                buffer(data),
                [this](boost::system::error_code ec, size_t /* length */) {
                    if (!ec){
                        do_client_read();
                    }
                    else{
                        std::cerr << "can't send to server" << std::endl;
                        _socket.close();
                        _client_socket.close();
                    }
                });
    }

    void do_client_read() {
        _client_socket.async_read_some(
                buffer(bytes, max_length),
                [this](boost::system::error_code ec, size_t length) {
                    if (!ec) {
                        std::string recv_str(bytes.data());
                        std::cout << "client read" << std::endl;
                        std::cout << recv_str << std::endl;
                        do_web_send(recv_str);
                    }
                    else{
                        _socket.close();
                        _client_socket.close();
                    }
                });
    }

    void do_web_read() {
        _socket.async_read_some(
                buffer(bytes, max_length),
                [this](boost::system::error_code ec, size_t length) {
                    if (!ec) {
                        std::string recv_str(bytes.data());
                        std::cout << "web read" << std::endl;
                        std::cout << recv_str << std::endl;
                        do_client_send(recv_str);
                    }
                    else{
                        _socket.close();
                        _client_socket.close();
                    }
                });
    }

    void do_connect(tcp::resolver::iterator it){
        // std::cout << "cmds.size() = " << cmds.size() << std::endl;
        _socket.async_connect(*it, [this](boost::system::error_code ec){
            if (!ec){
                do_client_read();
                do_web_read();
            }
            else{
                std::cerr << "do_connect failed: can't connect to target server" << std::endl;
            }
        });
    }

    void do_resolve() {
        //  std::cout << "cmds.size() = " << cmds.size() << std::endl;
        _resolv.async_resolve(_query, [this](boost::system::error_code ec, tcp::resolver::iterator it){
            if(!ec){
                do_connect(it);
            }
            else{
                std::cerr << "do_resolve failed: can't connect to np_server" << std::endl;
            }
        });
    }
};

class ClientSession : public enable_shared_from_this<ClientSession> {
 private:
  enum { max_length = 1024 };
  ip::tcp::socket _socket;
  array<char, max_length> _data;

 public:
  ClientSession(ip::tcp::socket socket) : _socket(move(socket)) {}

  void start() { do_read(); }

 private:
  void do_read() {
    auto self(shared_from_this());
    _socket.async_read_some(
        buffer(_data, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
          std::string recv_str(std::begin(_data), std::end(_data));

          if (is_sock(recv_str)){
              std::cout << "is sock" << std::endl;
              std::cout << recv_str << std::endl;
              SockRequest req = read_sock_request(
                      recv_str,_socket.remote_endpoint().address().to_string(),
                      std::to_string(_socket.remote_endpoint().port())
              );
              std::cout << req.get_msg() << std::endl;
              if (!ec){
//                  do_write(get_sock_reply(GRANTED, 0).to_str());
                    write(_socket, buffer(get_sock_reply(GRANTED, 0).to_str()));
              }
              else{
                  std::cout << "fail to reply" << std::endl;
              }

              WebSession ws(
                      ip_to_str(req.DSTIP), std::to_string(req.DSTPORT),
                      shared_from_this(),
                      move(_socket),
                      recv_str
              );
              ws.start();
              global_io_service.run();
          }
          else{
              std::cout << "not sock" << std::endl;
              std::cout << recv_str << std::endl;
          }
//          else{
//              WebRequest web_req = parse(
//                      recv_str,
//                      _socket.remote_endpoint().address().to_string(),
//                      std::to_string(_socket.remote_endpoint().port())
//              );
//              WebSession ws(
//                      web_req.http_host, "80",
//                      shared_from_this(),
//                      move(_socket),
//                      recv_str
//              );
//              ws.start();
//              global_io_service.run();
//          }

//          if(recv_str.substr(0, 3) == "GET"){
//              WebRequest web_req = parse(
//                      recv_str,
//                      _socket.remote_endpoint().address().to_string(),
//                      std::to_string(_socket.remote_endpoint().port())
//              );
//                  WebSession ws(
//                          web_req.http_host, "80",
//                          shared_from_this(),
//                          move(_socket),
//                          recv_str
//                  );
//                  ws.start();
//                  global_io_service.run();
//          }
//          else{
//              SockRequest req = read_sock_request(
//                      recv_str,_socket.remote_endpoint().address().to_string(),
//                      std::to_string(_socket.remote_endpoint().port())
//              );
//              std::cout << req.get_msg() << std::endl;
//              if (!ec) do_write(get_sock_reply(GRANTED, 0).to_str());
//          }
        });
  }

  void do_write(std::string data) {
    auto self(shared_from_this());
    _socket.async_send(
        buffer(data),
        [this, self](boost::system::error_code ec, size_t /* length */) {
          if (!ec) do_read();
          else{
              std::cerr << "async send failed" << std::endl;
          }
        });
  }
};

class SocksServer {
 private:
  ip::tcp::acceptor _acceptor;
  ip::tcp::socket _socket;
//  boost::asio::signal_set _signal;

 public:
  SocksServer(short port)
      : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service) {
    do_accept();
  }

 private:
  void do_accept() {
    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
      if (!ec){

          global_io_service.notify_fork(boost::asio::io_context::fork_prepare);

//           fork a child for client session
            std::cout << "new accept" << std::endl;
          if (fork() == 0){
              // child process
              global_io_service.notify_fork(boost::asio::io_context::fork_child);
              // child don't need listener
              _acceptor.close();

              // start client session
              make_shared<ClientSession>(move(_socket))->start();


          }

//          make_shared<ClientSession>(move(_socket))->start();
      }
        do_accept();
    });
  }
};

int main(int argc, char* const argv[]) {
  if (argc != 2) {
    cerr << "Usage:" << argv[0] << " [port]" << endl;
    return 1;
  }

  try {
    unsigned short port = atoi(argv[1]);
    SocksServer server(port);
    global_io_service.run();
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}