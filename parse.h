//
// Created by cysun on 12/31/19.
//

#ifndef SOCKSERVER_PARSE_H
#define SOCKSERVER_PARSE_H


#include <sstream>
#include <fstream>

#include <string>
#include <vector>

#include <boost/algorithm/string/replace.hpp>

typedef struct {
    std::string req_method;
    std::string req_uri;
    std::string remote_host;
    std::string server_protocol;
    std::string http_host;
    std::string remote_addr;
    std::string remote_port;
    std::string query_str;
} WebRequest;

WebRequest parse(std::string req_str, std::string remote_addr, std::string remote_port);
bool is_sock(std::string recv_str);
std::string ip_to_str(unsigned char ip[4]);

#endif //SOCKSERVER_PARSE_H
