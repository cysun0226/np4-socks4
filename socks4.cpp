#include "sock4.h"

bool invalidChar (char c){
    return !(c>=33 && c <126);
}
void stripUnicode(std::string & str){
    str.erase(remove_if(str.begin(),str.end(), invalidChar), str.end());
}


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

DstIP str_to_ip(std::string ip_str){
    std::stringstream ss;
    ss.str(ip_str);
    std::string token;
    DstIP dip;
    int i = 0;
    while(std::getline(ss, token, '.')){
        dip.ip[i] = (unsigned char) std::atoi(token.c_str());
        i++;
    }
    return dip;
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
    // if socks 4a, resolve domain name
    sp.domain_name = recv_str.substr(i);
    stripUnicode(sp.domain_name);
    if (sp.DSTIP[0] == 0 && sp.DSTIP[1] == 0 && sp.DSTIP[2] == 0){
        io_service ios;
        tcp::resolver  resolver(ios);
        tcp::resolver::query q(sp.domain_name, std::to_string(sp.DSTPORT));
        tcp::resolver::iterator iter = resolver.resolve(q);
        DstIP dip = str_to_ip(iter->endpoint().address().to_string());
        sp.DSTIP[0] = dip.ip[0];
        sp.DSTIP[1] = dip.ip[1];
        sp.DSTIP[2] = dip.ip[2];
        sp.DSTIP[3] = dip.ip[3];
    }

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
    "<D_PORT>:\t" << DSTPORT << std::endl <<
    "<Command>:\t" << cmd_to_str(CD) << std::endl <<
    "<Reply>:\t" << reply << std::endl;

    return ss.str();
}

std::string SockRequest ::to_str() {
    std::string request (9, 0);
    request[0] = VN;
    request[1] = CD;
    request[2] = DSTPORT >> 8;
    request[3] = DSTPORT & 0xFF ;
    request[4] = DSTIP[0];
    request[5] = DSTIP[1];
    request[6] = DSTIP[2];
    request[7] = DSTIP[3];
    request[8] = 0;
    request = request + domain_name;
    return request;
}

SockReply get_sock_reply(bool granted, unsigned short dstport, unsigned char dstip[4]){
    SockReply sr;
    sr.VN = 0;
    sr.CD = (granted)? 90 : 91;
    sr.DSTPORT = dstport;
    sr.DSTIP[0] = dstip[0];
    sr.DSTIP[1] = dstip[1];
    sr.DSTIP[2] = dstip[2];
    sr.DSTIP[3] = dstip[3];
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

bool is_sock(std::string recv_str){
    SockRequest sp;
    sp.VN = recv_str[0];
    sp.CD = recv_str[1];
    if (sp.VN == 4 && (sp.CD == 1 || sp.CD == 2)){
        return true;
    }
    return false;
}

