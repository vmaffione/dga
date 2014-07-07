#ifndef __MEMBER_HPP__
#define __MEMBER_HPP__

#include "remote.hpp"

#include <vector>
#include <stdint.h>
#include <string>

enum NodeColor { MPL_RED = 0, MPL_BLACK = 1 };

#define MANAGER_PORT    9863

class Member : public Remote {
    public:
        unsigned id;
        enum NodeColor color;

        Member() : Remote(), id(0), color(MPL_BLACK) { }
        Member(const Remote& r) : Remote(r), id(0), color(MPL_BLACK) { }
        Member(const Remote& r, unsigned _id, enum NodeColor _c)
                                    : Remote(r), id(_id), color(_c) { }
};

enum Opcode {
    JOIN = 1,
    LEAVE,
    UPDATE,
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

class LeaveRequest : public Message {
    public:
        std::string ip;
        uint32_t port;

        LeaveRequest() : port(0) { }
        LeaveRequest(const std::string& _ip, uint32_t _port);
        void serialize(RemoteConnection& remote) const;
        void deserialize(RemoteConnection& remote);
};

class UpdateRequest : public Message {
    public:
        std::vector<Member> members;

        UpdateRequest() { }
        void serialize(RemoteConnection& remote) const;
        void deserialize(RemoteConnection& remote);
};

class Response : public Message {
    public:
        std::string content;

        Response() { }
        Response(const std::string& _content);
        void serialize(RemoteConnection& remote) const;
        void deserialize(RemoteConnection& remote);
};

#endif  /* __MEMBER_HPP__ */
