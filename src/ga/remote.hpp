#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <string>
#include <list>

void exit_with_error(const char *errmsg);

class Remote {
    public:
        struct sockaddr_in address;
        std::string ip;
        short unsigned port;

        Remote();
        Remote(const struct sockaddr_in&);
};

class RemoteConnection {
    public:
        const Remote& remote;
        int fd;
        bool open;

        RemoteConnection(const Remote& r);
        RemoteConnection(int _fd, const Remote& r);
        int send_message(const char *buf, unsigned size) const;
        int recv_message(char *buf, unsigned size) const;
        int close();
};

class Server {
        short unsigned port;
        int listen_fd;
        struct sockaddr_in server_address;

    public:
        Server(short unsigned p);
        int run();
        virtual int process_request(const RemoteConnection& remote) = 0;
        virtual ~Server() { }
};
