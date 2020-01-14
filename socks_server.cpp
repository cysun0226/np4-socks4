#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "sock4.h"

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
//    std::array<char, 4096> bytes;
    std::array<char, 4096> web_data;
    std::array<char, 4096> client_data;
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
    void do_client_send(size_t length){
        _client_socket.async_send(
                buffer(web_data, length),
                [this](boost::system::error_code ec, size_t /* length */) {
                    if (!ec){
//                        std::cout << "# send to client: " << std::endl;
//                        std::cout << web_data.data();
                         do_web_read();
                    }
                    else{
                        _socket.close();
                        _client_socket.close();
                    }
                });
    }

    void do_web_send(size_t length){
        _socket.async_send(
                buffer(client_data, length),
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
                buffer(client_data, max_length),
                [this](boost::system::error_code ec, size_t length) {
                    if (!ec) {
//                        std::cout << client_data.data() << std::endl;
                        do_web_send(length);
                    }
                    else{
                        _socket.close();
                        _client_socket.close();
                    }
                });
    }

    void do_web_read() {
        _socket.async_read_some(
                buffer(web_data, max_length),
                [this](boost::system::error_code ec, size_t length) {
                    if (!ec) {
                        do_client_send(length);
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
                std::cerr << "do_resolve failed: can't connect to web server" << std::endl;
            }
        });
    }
};


class BindSession : public std::enable_shared_from_this<BindSession> {
private:
    tcp::socket _client_socket{global_io_service};
    tcp::socket _socket;
    tcp::acceptor _acceptor;
    std::array<char, 4096> web_data;
    std::array<char, 4096> client_data;
    std::shared_ptr<ClientSession> es_ptr;
    enum { max_length = 4096 };

public:
    // constructor
    BindSession(tcp::socket client_socket)
            : _client_socket(move(client_socket)),
              _socket(global_io_service),
              _acceptor(global_io_service, {tcp::v4(), 0})
    {
    }
    void start() {
        // sock reply
        write(_client_socket, buffer(
                get_sock_reply(BIND, _acceptor.local_endpoint().port(), str_to_ip("0.0.0.0").ip).to_str()));
        accept();
    }
private:
    void do_client_send(size_t length){
        async_write(_client_socket, buffer(web_data, length),
                [this](boost::system::error_code ec, size_t /* length */) {
                    if (!ec){
                         do_ftp_read();
                    }
                    else{
                        _client_socket.close();
                    }
                });
    }

     void do_client_read() {
         _client_socket.async_read_some(
                 buffer(client_data, max_length),
                 [this](boost::system::error_code ec, size_t length) {
                     if (!ec) {
                        //  std::cout << "# bind client_read " << std::endl;
                        //  std::cout << client_data.data() << std::endl;
                     }
                     else{
                         _socket.close();
                         _client_socket.close();
                     }
                 });
     }

    // void do_web_send(size_t length){
    //     _socket.async_send(
    //             buffer(client_data, length),
    //             [this](boost::system::error_code ec, size_t /* length */) {
    //                 if (!ec){
    //                     do_client_read();
    //                 }
    //                 else{
    //                     std::cerr << "can't send to server" << std::endl;
    //                     _socket.close();
    //                     _client_socket.close();
    //                 }
    //             });
    // }

    void do_ftp_read() {
        _socket.async_read_some(
                buffer(web_data, max_length),
                [this](boost::system::error_code ec, size_t length) {
                    if (!ec) {
                        do_client_send(length);
                    }
                    else{
                        _socket.close();
                        _client_socket.close();
                    }
                });
    }

    void accept(){
    _acceptor.async_accept(
        [this](boost::system::error_code ec, tcp::socket new_socket)
        {
          if (!ec){
            _socket = std::move(new_socket);
            // sock reply
            write(_client_socket, buffer(
                  get_sock_reply(BIND, _acceptor.local_endpoint().port(), str_to_ip("0.0.0.0").ip).to_str()));
            do_ftp_read();
//            accept();

          }
          else
          {
            std::cerr << "Accept error: " << ec.message() << std::endl;
            accept();
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
              std::string src_ip = _socket.remote_endpoint().address().to_string();
              std::string src_port = std::to_string(_socket.remote_endpoint().port());
              SockRequest req = read_sock_request(
                      recv_str,src_ip,
                      src_port
              );
              std::cout << req.get_msg() << std::endl;
              if (!ec){
                    // sock reply
                    if(req.CD == CONNECT){
                        write(_socket, buffer(get_sock_reply(CONNECT, 0, req.DSTIP).to_str()));
                    }
                    else if (req.CD == BIND){
                        BindSession bs(move(_socket));
                        bs.start();
                        global_io_service.run();          
                    }
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
          }
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
  boost::asio::signal_set _signal;

 public:
  SocksServer(short port)
      : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service),
        _signal(global_io_service, SIGCHLD)
        {
    wait_for_signal();
    read_config();
    do_accept();
  }

 private:

  void wait_for_signal(){
      _signal.async_wait(
              [this](boost::system::error_code /*ec*/, int /*signo*/)
              {
                  if (_acceptor.is_open())
                  {
                      // Reap completed child processes so that we don't end up with
                      // zombies.
                      int status = 0;
                      while (waitpid(-1, &status, WNOHANG) > 0) {}

                      wait_for_signal();
                  }
              });
  }

  void do_accept() {
    _acceptor.async_accept(
            [this](boost::system::error_code ec, tcp::socket new_socket){
      if (!ec){
            _socket = std::move(new_socket);
          global_io_service.notify_fork(boost::asio::io_context::fork_prepare);
            // fork a child for client session
            std::cout << "new accept" << std::endl;


          if (fork() == 0){
              // child process
              global_io_service.notify_fork(boost::asio::io_context::fork_child);
              // child don't need listener
              _acceptor.close();
              _signal.cancel();

              // start client session
              make_shared<ClientSession>(move(_socket))->start();
          }
          else{
              global_io_service.notify_fork(boost::asio::io_context::fork_parent);
              _socket.close();
              do_accept();
          }

//          make_shared<ClientSession>(move(_socket))->start();
      }
      else{
          std::cerr << "Accept error: " << ec.message() << std::endl;
          do_accept();
      }

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