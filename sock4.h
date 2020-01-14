#ifndef SOCKS4_H
#define SOCKS4_H

#include <iostream>
#include <string>
#include <sstream>
#include <boost/asio/signal_set.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <boost/asio.hpp>
#include <fstream>

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define CONFIG_FILE "socks.conf"

using namespace boost::asio;
using namespace ip;

enum COMMAND{CONNECT=1, BIND=2};
enum RESULT{GRANTED=1, REJECTED=2};

typedef struct {
    unsigned char VN;
    unsigned char CD;
    unsigned short DSTPORT;
    unsigned char DSTIP[4];
    std::string other = "";
    std::string domain_name;
    std::string src_ip;
    std::string src_port;
    std::string reply;
    std::string get_msg();
    std::string to_str();
} SockRequest;

typedef struct {
    unsigned char VN;
    unsigned char CD;
    unsigned short DSTPORT;
    unsigned char DSTIP[4];
    std::string to_str();
} SockReply;

typedef struct {
    unsigned char ip[4];
} DstIP;

typedef struct {
    int mode;
    std::string ip;
} Rule;

SockRequest read_sock_request(std::string recv_str, std::string src_ip, std::string src_port);

SockReply get_sock_reply(int mode, unsigned short dstport, unsigned char dstip[4]);

DstIP str_to_ip(std::string ip_str);

void read_config();

bool is_sock(std::string recv_str);

std::string ip_to_str(unsigned char ip[4]);

#endif
