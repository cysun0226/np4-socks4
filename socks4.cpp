#include "sock4.h"

std::string cmd_to_str(unsigned char CD){
    switch (CD) {
        case CONNECT:
            return "CONNECT";
        case BIND:
            return "BIND";
    }
    return "Not Defined";
}

std::string ip_to_str(unsigned char ip[4]){
    std::stringstream ss;
    ss <<
    std::to_string(ip[0]) << "." <<
    std::to_string(ip[1]) << "." <<
    std::to_string(ip[2]) << "." <<
    std::to_string(ip[3]);
    return ss.str();
}

SockRequest read_sock_request(std::string recv_str, std::string src_ip, std::string src_port){
    SockRequest sp;
    sp.VN = recv_str[0];
    sp.CD = recv_str[1];
    sp.DSTPORT = (((unsigned short)recv_str[2]) << 8) | (0x00ff & recv_str[3]);
    sp.DSTIP[0] = recv_str[4];
    sp.DSTIP[1] = recv_str[5];
    sp.DSTIP[2] = recv_str[6];
    sp.DSTIP[3] = recv_str[7];
    int i = 8;
    while (recv_str.size()>=8 && recv_str[i] != 0){
        sp.other.push_back(recv_str[i]);
    }
    // domain name
    sp.domain_name = recv_str.substr(i);
    // src info
    sp.src_ip = src_ip;
    sp.src_port = src_port;
    sp.reply = "Accept";

    return sp;
}

std::string SockRequest::get_msg() {
    std::stringstream ss;
    ss << "\n[ Socks request ]\n" << std::endl <<
    "<S_IP>:\t" << src_ip << std::endl <<
    "<S_PORT>:\t" << src_port << std::endl <<
    "<D_IP>:\t" << ip_to_str(DSTIP) << std::endl <<
    "<D_PORT>:\t" << src_port << std::endl <<
    "<Command>:\t" << cmd_to_str(CD) << std::endl <<
    "<Reply>:\t" << reply << std::endl;

    return ss.str();
}

SockReply get_sock_reply(bool granted, unsigned short dstport){
    SockReply sr;
    sr.VN = 0;
    sr.CD = (granted)? 90 : 91;
    sr.DSTPORT = dstport;
    sr.DSTIP[0] = 0;
    sr.DSTIP[1] = 0;
    sr.DSTIP[2] = 0;
    sr.DSTIP[3] = 0;
    return sr;
}

std::string SockReply::to_str() {
    std::string reply (8, 0);
    reply[0] = VN;
    reply[1] = CD;
    reply[2] = DSTPORT >> 8;
    reply[3] = DSTPORT & 0xFF ;
    reply[4] = DSTIP[0];
    reply[5] = DSTIP[1];
    reply[6] = DSTIP[2];
    reply[7] = DSTIP[3];

    return reply;
}