#include <iostream>
#include <csignal>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <string>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <csignal>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <sys/types.h>
#include <sys/wait.h>

#include "sock4.h"

#define MAX_SESSION 5
std::string web_host_name[MAX_SESSION];
std::string host_port[MAX_SESSION];
std::string file_name[MAX_SESSION];
std::string socks_host;
std::string socks_port;
std::vector <std::string> cmds[MAX_SESSION+1];
int host_num = 0;

// ==================================================================================
// np client
// ==================================================================================
using namespace boost::asio;
using namespace boost::asio::ip;

io_service ioservice;

std::string change_escape(std::string cmd){
  boost::replace_all(cmd, "&", "&amp;");
  boost::replace_all(cmd, "\"", "&quot;");
  boost::replace_all(cmd, "\'", "&apos;");
  boost::replace_all(cmd, "<", "&lt;");
  boost::replace_all(cmd, ">", "&gt;");
  boost::replace_all(cmd, "\n", "&NewLine;");
  boost::replace_all(cmd, "\r", "");
  return cmd;
}

void output_command(std::string sid, std::string cmd){
    cmd = change_escape(cmd);
    // boost::replace_all(cmd, "\n", "&NewLine");
    std::cout << "<script>document.getElementById('s" << 
    sid << "').innerHTML += '<b>" << cmd << "&NewLine;</b>';</script>" << std::endl;
    std::cout.flush();
}

void output_prompt(std::string sid){
    std::cout << "<script>document.getElementById('s" << 
    sid << "').innerHTML += '<b>" << "% " << "</b>';</script>" << std::endl;
    std::cout.flush();
}

void output_shell(std::string sid, std::string str){
    str = change_escape(str);
    // if(str == ""){
    //     return;
    // }
    std::cout << "<script>document.getElementById('s" << sid << 
                 "').innerHTML += '" << str <<"';</script>" << std::endl;
    std::cout.flush();
}

class ShellSession : public std::enable_shared_from_this<ShellSession> {
    private:
        tcp::resolver _resolv{ioservice};
        tcp::socket _socket{ioservice};
        tcp::resolver::query _query;
        // std::vector<std::string> cmds;
        std::array<char, 4096> _cmd_buffer;
        int cmd_idx;
        std::string session_id;
        int s_id;
        std::string host_ip;
        std::string host_port;
        std::string cmd_file;
        enum { max_length = 4096 };
        std::array<char, 4096> bytes;
        
    public:
        // constructor
        ShellSession(std::string host_ip, std::string host_port, std::string cmd_file, std::string sid)
            : _query{socks_host, socks_port},
              host_ip(host_ip),
              host_port(host_port),
              cmd_file(cmd_file),
              session_id(sid),
              cmd_idx(0),
              s_id(std::atoi(sid.c_str()))
        {
            read_cmd_from_file();
        }
        void start() { do_resolve(); }
    private:
        bool read_cmd_from_file(){
            std::string cmd_line;
            std::ifstream input_file("./test_case/"+std::string(cmd_file));
            if(input_file.is_open()){
                while ( getline (input_file, cmd_line)){
                    cmds[s_id].push_back(cmd_line);
                }
                input_file.close();
                return true;
            }
            else{
                std::cout << "<p> can't open " << cmd_file << "</p>" << std::endl;
                return false;
            }
        }

        void do_send_cmd(){
              _socket.async_send(
                buffer(cmds[s_id][cmd_idx]+"\r\n"),
                [this](boost::system::error_code ec, size_t /* length */) {
                    if (!ec){
                        cmd_idx++;
                        do_read();
                    } 
                });
        }

        void do_read() {
            _socket.async_read_some(
                buffer(bytes, max_length),
                [this](boost::system::error_code ec, size_t length) {
                if (!ec){
                    std::string recv_str(bytes.data());
                    bytes.fill(0);
                    // std::cout << recv_str;
                    // std::cout.flush();

                    if (recv_str.find("%") != std::string::npos){
                        
                        // std::cout << "<script>document.getElementById('s0').innerHTML += '';</script>" << std::endl;
                        // std::cout << recv_str.substr(0, recv_str.find("%")) << std::endl;
                        // usleep(100000);
                        output_shell(std::to_string(s_id), recv_str.substr(0, recv_str.find("%")));
                        output_prompt(std::to_string(s_id));
                    }
                    else{
                        // if(bytes[0] != 0)
                        // std::cout << "# get data" << std::endl;
                        // std::cout << recv_str << std::endl;
                        output_shell(std::to_string(s_id), recv_str);
                    }
                    
                    // find prompt, send command
                    if (recv_str.find("%") != std::string::npos){ // find prompt
                        usleep(100000);
                        
                        // std::cout << "find %, cmds.size() = " << cmds.size() << std::endl;
                        // std::cout << cmds[s_id][cmd_idx] << std::endl;
                        std::string r = cmds[s_id][cmd_idx] + "\r\n";
                        output_command(std::to_string(s_id), cmds[s_id][cmd_idx]);

                        usleep(100000);
                        do_send_cmd();
                    }
                    else{
                        // std::cout.write(bytes.data(), bytes_transferred);
                        // std::cout.flush();
                        // output_shell(session_id, recv_str);
                        do_read();
                    }
                }
                else{
                    output_command(std::to_string(s_id), "(connection close)");
                }
            });
        }

        void do_connect(tcp::resolver::iterator it){
          // std::cout << "cmds.size() = " << cmds.size() << std::endl;
            _socket.async_connect(*it, [this](boost::system::error_code ec){
                if (!ec){
                    // connect to the socks server
                    SockRequest sr;
                    sr.VN = 4;
                    sr.CD = 1;
                    sr.DSTPORT = (unsigned short)std::atoi(host_port.c_str());
                    sr.DSTIP[0] = 0;
                    sr.DSTIP[1] = 0;
                    sr.DSTIP[2] = 0;
                    sr.DSTIP[3] = 1;
                    sr.domain_name = host_ip;
                    write(_socket, buffer(sr.to_str()));
                    do_read();
                }
                else{
                    std::cerr << "can't connect to sock server" << std::endl;
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
                    std::cerr << "can't connect to np_server" << std::endl;
                }
            });
        }
};


const std::string webpage_template = R"(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>NP Project 3 Console</title>
    <link
      rel="stylesheet"
      href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css"
      integrity="sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO"
      crossorigin="anonymous"
    />
    <link
      href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
      rel="stylesheet"
    />
    <link
      rel="icon"
      type="image/png"
      href="https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png"
    />
    <style>
      * {
        font-family: 'Source Code Pro', monospace;
        font-size: 1rem !important;
      }
      body {
        background-color: #212529;
      }
      pre {
        color: #cccccc;
      }
      b {
        color: #ffffff;
      }
    </style>
  </head>
  <body>
    <table class="table table-dark table-bordered">
      <thead>
        <tr>
)";

std::vector<std::string>  host_cols;
std::vector<std::string>  table_tds;

          
std::string generate_webpage(){
  

  const std::string footer_1 = "</tr>\n</thead>\n<tbody>\n<tr>";
  const std::string footer_2 = "</tr>\n</tbody>\n</table>\n</body>\n</html>";

  std::string page = webpage_template;
  for (size_t i = 0; i < host_num; i++){
    page += host_cols[i];
  }
  page += footer_1;
  for (size_t i = 0; i < host_num; i++){
    page += table_tds[i];
  }
  page += footer_2;
  return page;
}
        


void parse_query(std::string query_str){
  std::stringstream ss;
  ss.str(query_str);
  std::string str;

  std::vector<std::string> tokens;
  boost::split( tokens, query_str, boost::is_any_of( "&" ), boost::token_compress_on);
  for( std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it ){
    if(it->at(0) == 'h'){
      std::string hid = it->substr(0, it->find("="));
      web_host_name[atoi(hid.substr(1).c_str())] = it->substr(it->find("=")+1);
      if(it->substr(it->find("=")+1).size()>1){
        host_num++;
      }
    }
    if(it->at(0) == 'p'){
      std::string pid = it->substr(0, it->find("="));
      host_port[atoi(pid.substr(1).c_str())] = it->substr(it->find("=")+1);
    }
    if(it->at(0) == 'f'){
      std::string fid = it->substr(0, it->find("="));
      file_name[atoi(fid.substr(1).c_str())] = it->substr(it->find("=")+1);
    }

    // sock server & port
      if(it->at(0) == 's' && it->at(1) == 'h'){
          socks_host = it->substr(it->find("=")+1);
      }
      if(it->at(0) == 's' && it->at(1) == 'p'){
          socks_port = it->substr(it->find("=")+1);
      }
  }
}


int main(int argc, char** argv) {
    char* ptr = getenv("QUERY_STRING");
    std::string query_str(ptr);
    parse_query(query_str);

    for (size_t i = 0; i < host_num; i++){
      host_cols.push_back("<th scope=\"col\">"+web_host_name[i]+":"+host_port[i]+"</th>");
      table_tds.push_back("<td><pre id=\"s"+std::to_string(i)+"\" class=\"mb-0\"></pre></td>");
    }

    std::cout << "Content-type: text/html" << std::endl << std::endl;
    std::cout << generate_webpage() << std::endl;
    
    // build np_server connection
    std::vector<ShellSession> np_sessions;
    for (size_t i = 0; i < host_num; i++){
      np_sessions.push_back(ShellSession(web_host_name[i], host_port[i], file_name[i], std::to_string(i)));
    }
    for (size_t i = 0; i < host_num; i++){
      np_sessions[i].start();
    }

    ioservice.run();

    return 0;
}