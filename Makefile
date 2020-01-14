CXX=clang++
CXXFLAGS=-std=c++11 -Wall -pedantic -pthread -lboost_system
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))

all: socks_server hw4.cgi

socks_server_obj = ./socks_server.o ./socks4.o
hw4_obj = ./hw4.o socks4.o

socks_server : $(socks_server_obj)
	clang++ -o socks_server -I /usr/local/include -L /usr/local/lib -std=c++11 -Wall -pedantic -pthread -lboost_system $(socks_server_obj)

hw4.cgi : $(hw4_obj)
	clang++ -o hw4.cgi -I /usr/local/include -L /usr/local/lib -std=c++11 -Wall -pedantic -pthread -lboost_system $(hw4_obj)

./socks4.o : ./sock4.h

# socks_server = $(CXX) socks_server.cpp socks4.cpp parse.cpp -o socks_server $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

clean:
	rm -f *.cgi
	rm -f *.o
	rm -f socks_server
