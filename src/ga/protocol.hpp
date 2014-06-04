#ifndef __MEMBER_HPP__
#define __MEMBER_HPP__

#include "remote.hpp"


enum NodeColor { MPL_RED = 0, MPL_BLACK = 1 };

#define MANAGER_PORT    9863

class Member : public Remote {
    public:
        unsigned id;
        enum NodeColor color;

        Member() : Remote(), color(MPL_BLACK), id(0) { }
        Member(const Remote& r) : Remote(r), color(MPL_BLACK), id(0) { }
};

enum Opcode {
        JOIN = 1,
};

class JoinRequest : public Message {
    public:
        std::string ip;
        uint32_t port;

        JoinRequest() : port(0) { }
        JoinRequest(const std::string& _ip, uint32_t _port);
        void serialize(RemoteConnection& remote) const;
        void deserialize(RemoteConnection& remote);
};

#endif  /* __MEMBER_HPP__ */
