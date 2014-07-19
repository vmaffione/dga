#ifndef __MEMBER_HPP__
#define __MEMBER_HPP__

#include "remote.hpp"

#include <vector>
#include <stdint.h>
#include <string>


#define MANAGER_PORT    9863

class Member : public Remote {
    public:
        Member() : Remote() { }
        Member(const Remote& r) : Remote(r) { }
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
        bool add;

        UpdateRequest() : add(true) { }
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
