#ifndef __REMOTE_HPP__
#define __REMOTE_HPP__

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
#include <stdint.h>


void exit_with_error(const char *errmsg);

class Remote {
    public:
        struct sockaddr_in address;
        std::string ip;
        uint16_t port;

        Remote();
        Remote(const struct sockaddr_in&);
        Remote(std::string _ip, uint16_t _port);
        bool operator==(const Remote&) const;
        bool operator<(const Remote&) const;
};

class RemoteConnection {
    public:
        const Remote& remote;
        int fd;
        bool open;

        RemoteConnection(const Remote& r);
        RemoteConnection(int _fd, const Remote& r);
        int send_message(const void *buf, unsigned size) const;
        int recv_message(void *buf, unsigned size) const;
        int close();

        static const int MaxSize = 255;
        void serialize(const std::string& str);
        void deserialize(std::string& str);
        void serialize(uint8_t byte);
        void deserialize(uint8_t& byte);
        void serialize(uint32_t dw);
        void deserialize(uint32_t& dw);
        void serialize(const char *src, const unsigned int len);
        void deserialize(char *dst, unsigned int avail, unsigned int& len);
};

class Message {
    public:
        virtual void serialize(RemoteConnection& remote) const = 0;
        virtual void deserialize(RemoteConnection& remote) = 0;
};

class Server {
        uint16_t port;
        int listen_fd;
        struct sockaddr_in server_address;

    public:
        Server(uint16_t p);
        int run();
        virtual int process_request(RemoteConnection& remote) = 0;
        virtual ~Server() { }
};

#endif  /*__REMOTE_HPP__ */
