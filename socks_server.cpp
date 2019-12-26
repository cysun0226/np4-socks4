#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "sock4.h"

using namespace std;
using namespace boost::asio;

io_service global_io_service;

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
          SockRequest req = read_sock_request(
              recv_str,_socket.remote_endpoint().address().to_string(),
      std::to_string(_socket.remote_endpoint().port())
          );
          std::cout << req.get_msg() << std::endl;
           if (!ec) do_write(get_sock_reply(GRANTED, 0).to_str());
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

 public:
  SocksServer(short port)
      : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service) {
    do_accept();
  }

 private:
  void do_accept() {
    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
      if (!ec) make_shared<ClientSession>(move(_socket))->start();

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