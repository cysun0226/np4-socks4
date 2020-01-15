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

std::vector<Rule> firewall_rule;

void read_config();

std::vector<Rule> access_record;

bool is_permit(int mode, unsigned char dstip[4]){
    read_config();
    for (int i = 0; i < firewall_rule.size(); ++i) {
        std::string permit_ip[4];
        std::stringstream ss;
        ss.str(firewall_rule[i].ip);
        std::string token;
        int t = 0;
        while(std::getline(ss, token, '.')){
            permit_ip[t] = token;
            t++;
        }

        int match_count = 0;

        for (int j = 0; j < 4; ++j) {
            if (std::to_string(dstip[j]) == permit_ip[j] || permit_ip[j] == "*"){ // same or is match
                match_count ++;
            }
        }

        if (match_count == 4 && mode == firewall_rule[i].mode){
            bool has_record = false;
            for (size_t a = 0; a < access_record.size(); a++){
                if (ip_to_str(dstip) == access_record[a].ip){
                    access_record[a].mode ++;
                    bool has_record = true;
                }
            }

            if(has_record != true){
                Rule record;
                record.ip = ip_to_str(dstip);
                record.mode = 0;
                access_record.push_back(record);
            }

            return true;
        }
    }
    return false;
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
    sp.domain_name = sp.domain_name.substr(0, sp.domain_name.find("@"));
    if (sp.DSTIP[0] == 0 && sp.DSTIP[1] == 0 && sp.DSTIP[2] == 0){
        struct addrinfo hints, *res, *p;
        int status;
        char ipstr[INET6_ADDRSTRLEN];

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((status = getaddrinfo(sp.domain_name.c_str(), std::to_string(sp.DSTPORT).c_str(), &hints, &res)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
            exit(1);
        }

        struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
        void *addr = &(ipv4->sin_addr);
        char *ipver = "IPv4";
        inet_ntop(res->ai_family, addr, ipstr, sizeof ipstr);
        // std::cout << ipstr << std::endl;

//        io_service ios;
//        tcp::resolver  resolver(ios);
//        tcp::resolver::query q(sp.domain_name, std::to_string(sp.DSTPORT));
//        tcp::resolver::iterator iter = resolver.resolve(q);
//        DstIP dip = str_to_ip(iter->endpoint().address().to_string());
//        std::cout << sp.domain_name << std::endl;
//        std::cout << iter->endpoint().address().to_string() << std::endl;
        DstIP dip = str_to_ip(std::string(ipstr));
        sp.DSTIP[0] = dip.ip[0];
        sp.DSTIP[1] = dip.ip[1];
        sp.DSTIP[2] = dip.ip[2];
        sp.DSTIP[3] = dip.ip[3];
//        freeaddrinfo(res);
    }

    // src info
    sp.src_ip = src_ip;
    sp.src_port = src_port;
    sp.reply = "Accept";

    return sp;
}

std::string SockRequest::get_msg() {
    std::stringstream ss;
    std::string reply = (is_permit(CD, DSTIP))? "Accept" : "Reject";
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
    std::vector<char> request;
    request.push_back(VN);
    request.push_back(CD);
    request.push_back(DSTPORT >> 8);
    request.push_back(DSTPORT & 0xFF);
    request.push_back(DSTIP[0]);
    request.push_back(DSTIP[1]);
    request.push_back(DSTIP[2]);
    request.push_back(DSTIP[3]);
    request.push_back(0);
    for (size_t i = 0; i < domain_name.size(); i++){
        request.push_back(domain_name[i]);
    }
    std::string request_str(request.size(), 0);
    for (size_t i = 0; i < request.size(); i++){
        request_str[i] = request[i];
    }
    const int length = request.size();
    return request_str;
}

SockReply get_sock_reply(int mode, unsigned short dstport, unsigned char dstip[4]){
    SockReply sr;
    sr.VN = 0;
    sr.CD = (is_permit(mode, dstip))? 90 : 91;
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

bool has_rule(){
    for (int i = 0; i < firewall_rule.size(); i++){
	if (firewall_rule[i].ip != "*.*.*.*"){
	    return true;
	}
    }
    return false;
}

void read_config(){
    firewall_rule.clear();
    std::ifstream in_file(CONFIG_FILE);
    std::string line;
    if (in_file){
        while (std::getline(in_file, line)){
            std::stringstream ss;
            std::string tmp;
            Rule rule;
            ss.str(line);
            ss >> tmp; // permit
            ss >> tmp; // mode
            rule.mode = (tmp == "b")? BIND : CONNECT;
            ss >> rule.ip; // ip
            firewall_rule.push_back(rule);
        }
	
	if(has_rule()){

        std::cout << "\n # Permit IP list\n" << std::endl;

        for (int i = 0; i < firewall_rule.size(); ++i) {
            std::string m = (firewall_rule[i].mode == CONNECT)? "c":"b";
            std::cout << m << " ";
            std::cout << firewall_rule[i].ip << std::endl;
        }
	}
    }
    else{
        std::cerr << "can't open sock config: " << CONFIG_FILE << std::endl;
    }
}
