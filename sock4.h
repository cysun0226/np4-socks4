#ifndef SOCKS4_H
#define SOCKS4_H

#include <iostream>
#include <string>
#include <sstream>

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
} SockRequest;

typedef struct {
    unsigned char VN;
    unsigned char CD;
    unsigned short DSTPORT;
    unsigned char DSTIP[4];
    std::string to_str();
} SockReply;

SockRequest read_sock_request(std::string recv_str, std::string src_ip, std::string src_port);

SockReply get_sock_reply(bool granted, unsigned short dstport);

#endif
