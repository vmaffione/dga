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


class Member
{
    public:
        string ip;
        unsigned port;
        NodeColor color;
        unsigned id;

        Member();
        Member(const struct sockaddr_in&);
};

Member::Member() : ip("0.0.0.0"), port(0), color(MPL_BLACK), id(0)
{
}

Member::Member(const struct sockaddr_in& address)
{
        char ipbuf[INET_ADDRSTRLEN];

        port = ntohs(address.sin_port);

        memset(ipbuf, 0, sizeof(ipbuf));
        inet_ntop(AF_INET, &(address.sin_addr),
                  ipbuf, INET_ADDRSTRLEN);

        ip = string(ipbuf);
}

class Manager
{
        list<Member> members;
        NodeColor next_color;
        unsigned next_id;

    public:
        Manager();
        void add_member(const Member& member);
};

Manager::Manager() : next_color(MPL_BLACK), next_id(1)
{
}

void Manager::add_member(const Member& member)
{
        members.push_back(member);
        members.back().color = next_color;
        members.back().id = next_id;

        if (next_color == MPL_BLACK) {
                next_color = MPL_RED;
        } else {
                next_color = MPL_BLACK;
        }

        next_id++;
}

void exit_with_error(const char *errmsg)
{
        perror(errmsg);
        exit(EXIT_FAILURE);
}

int manage_request(Manager& manager, int fd,
                   const struct sockaddr_in& address)
{
#define BUFSIZE 128
        char buffer[BUFSIZE];
        int n;
        Member member(address);

        cout << "Request received: " <<
                member.ip << ":"
                << member.port << "\n";

        n = read(fd, buffer, sizeof(buffer));
        if (n < 0) {
                exit_with_error("read()");
        }

        manager.add_member(member);

        n = write(fd, buffer, n);
        if (n < 0) {
                exit_with_error("write()");
        }

        return 0;
}

int main()
{
        int listen_fd;
        short int port = 9863;
        struct sockaddr_in server_address;
        Manager manager;
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

        for (;;) {
                int connection_fd;
                struct sockaddr_in client_address;
                size_t address_len = sizeof(client_address);

                connection_fd = accept(listen_fd,
                                       (struct sockaddr *)&client_address,
                                       &address_len);
                if (connection_fd < 0) {
                        exit_with_error("accept()");
                }

                manage_request(manager, connection_fd, client_address);

                ret = close(connection_fd);
                if (ret < 0) {
                        exit_with_error("close()");
                }
        }

        return 0;
}
