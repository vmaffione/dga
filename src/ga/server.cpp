#include "common.hpp"

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

using namespace std;


void exit_with_error(const char *errmsg)
{
        perror(errmsg);
        exit(EXIT_FAILURE);
}

class Remote {
    public:
        struct sockaddr_in address;
        string ip;
        short unsigned port;

        Remote();
        Remote(const struct sockaddr_in&);
};

Remote::Remote() : ip("0.0.0.0"), port(0)
{
        memset(&address, 0, sizeof(address));
}

Remote::Remote(const struct sockaddr_in& a) : address(a)
{
        char ipbuf[INET_ADDRSTRLEN];

        port = ntohs(address.sin_port);

        memset(ipbuf, 0, sizeof(ipbuf));
        inet_ntop(AF_INET, &(address.sin_addr),
                  ipbuf, INET_ADDRSTRLEN);

        ip = string(ipbuf);
}

class RemoteConnection {
    public:
        const Remote& remote;
        int fd;
        bool open;

        RemoteConnection(int _fd, const Remote& r);
        int send_message(const char *buf, unsigned size) const;
        int recv_message(char *buf, unsigned size) const;
        int close();
};

RemoteConnection::RemoteConnection(int _fd, const Remote& r) : remote(r)
{
        fd = _fd;
        open = true;
}

int RemoteConnection::close()
{
        int ret = ::close(fd);

        open = false;

        if (ret < 0) {
                exit_with_error("close()");
        }

        return ret;
}

int RemoteConnection::send_message(const char *buf, unsigned size) const
{
        int n = write(fd, buf, size);

        if (n < 0) {
                exit_with_error("write()");
        }

        return n;
}

int RemoteConnection::recv_message(char *buf, unsigned size) const
{
        int n = read(fd, buf, size);

        if (n < 0) {
                exit_with_error("read()");
        }

        return n;
}

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

Server::Server(short unsigned p) : port(p)
{
        int optval;
        int ret;

        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) {
                exit_with_error("creating listening socket");
        }

        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
        server_address.sin_port = htons(port);

        optval = 1;
        ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                         &optval, sizeof(optval));
        if (ret < 0) {
                exit_with_error("setsockopt()");
        }

        ret = bind(listen_fd, (struct sockaddr *)&server_address,
                        sizeof(server_address));
        if (ret < 0) {
                exit_with_error("bind()");
        }

        ret = listen(listen_fd, 10);
        if (ret < 0) {
                exit_with_error("listen()");
        }
}

int Server::run()
{
        for (;;) {
                int connection_fd;
                struct sockaddr_in client_address;
                size_t address_len = sizeof(client_address);
                int ret;

                connection_fd = accept(listen_fd,
                                       (struct sockaddr *)&client_address,
                                       &address_len);
                if (connection_fd < 0) {
                        exit_with_error("accept()");
                }

                Remote remote(client_address);
                RemoteConnection connection(connection_fd, remote);

                process_request(connection);
                connection.close();
        }

        return 0;
}

/* =================================================================== */

class Member : public Remote {
    public:
        unsigned id;
        enum NodeColor color;

        Member();
        Member(const Remote& r);
};

Member::Member() : Remote(), color(MPL_BLACK), id(0)
{
}

Member::Member(const Remote& r) : Remote(r), color(MPL_BLACK), id(0)
{
}

class Manager
{
        list<Member> members;
        NodeColor next_color;
        unsigned next_id;

    public:
        Manager();
        void add_member(const Remote& remote);
};

Manager::Manager() : next_color(MPL_BLACK), next_id(1)
{
}

void Manager::add_member(const Remote& remote)
{
        Member member(remote);

        member.color = next_color;
        member.id = next_id;

        members.push_back(member);

        if (next_color == MPL_BLACK) {
                next_color = MPL_RED;
        } else {
                next_color = MPL_BLACK;
        }

        next_id++;
}

class ManagerServer : public Server {
        Manager manager;

    public:
        ManagerServer(short unsigned port) : Server(port) { }
        virtual int process_request(const RemoteConnection& connection);
};

int ManagerServer::process_request(const RemoteConnection& connection)
{
#define BUFSIZE 128
        char buffer[BUFSIZE];
        int n;

        cout << "Request received: " <<
                connection.remote.ip << ":"
                << connection.remote.port << "\n";

        n = connection.recv_message(buffer, sizeof(buffer));

        manager.add_member(connection.remote);

        n = connection.send_message(buffer, n);

        return 0;
}

int main()
{
        ManagerServer server(9863);

        server.run();

        return 0;
}
