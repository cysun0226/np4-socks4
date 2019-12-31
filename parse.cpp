//
// Created by cysun on 12/31/19.
//

#include "parse.h"

WebRequest parse(std::string req_str, std::string remote_addr, std::string remote_port){
    boost::replace_all(req_str, "\\", "");
    // std::cout << req_str << std::endl;
    std::stringstream ss;
    ss.str(req_str);
    std::string str;
    WebRequest web_req;

    while (ss >> str) {
        if (str == "GET"){
            web_req.req_method = "GET";
            // uri & QUERY STRING
            ss >> str;
            web_req.req_uri = str.substr(0, str.find("?"));
            web_req.query_str = (str.find("?") != std::string::npos)? str.substr(str.find("?")+1) : "";
            // SERVER PROTOCOL
            ss >> web_req.server_protocol;
        }
        if (str == "Host:"){
            ss >> web_req.http_host;
        }
    }

    web_req.remote_addr = remote_addr;
    web_req.remote_port = remote_port;

    return  web_req;
}
