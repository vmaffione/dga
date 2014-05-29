#include "remote.hpp"

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

RemoteConnection::RemoteConnection(int _fd, const Remote& r) : remote(r)
{
        fd = _fd;
        open = true;
}

RemoteConnection::RemoteConnection(const Remote& r) : remote(r)
{
        int ret;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
                exit_with_error("socket()");
        }

        ret = connect(fd, (struct sockaddr *)&remote.address,
                        sizeof(remote.address));
        if (ret < 0) {
                exit_with_error("connect()");
        }

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
        int n;

        if (!open) {
                cerr << __func__ << ": cannot send to a closed connection\n";
                return 0;
        }

        n = write(fd, buf, size);
        if (n < 0) {
                exit_with_error("write()");
        }

        return n;
}

int RemoteConnection::recv_message(char *buf, unsigned size) const
{
        int n;

        if (!open) {
                cerr << __func__ <<
                        ": cannot receive from a closed connection\n";
                return 0;
        }

        n = read(fd, buf, size);
        if (n < 0) {
                exit_with_error("read()");
        }

        return n;
}

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

Remote::Remote(std::string _ip, short unsigned _port) : ip(_ip), port(_port)
{
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &address.sin_addr);
        address.sin_port = htons(port);
}
