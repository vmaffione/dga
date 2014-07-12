#include "protocol.hpp"

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <sstream>

using namespace std;


/* JoinRequest */
JoinRequest::JoinRequest(const string& _ip, uint32_t _port) :
    ip(_ip), port(_port)
{
}

void JoinRequest::serialize(RemoteConnection& remote) const
{
    remote.serialize(static_cast<uint8_t>(JOIN));
    remote.serialize(ip);
    remote.serialize(port);
}

void JoinRequest::deserialize(RemoteConnection& remote)
{
    remote.deserialize(ip);
    remote.deserialize(port);
}

/* LeaveRequest */
LeaveRequest::LeaveRequest(const string& _ip, uint32_t _port) :
    ip(_ip), port(_port)
{
}

void LeaveRequest::serialize(RemoteConnection& remote) const
{
    remote.serialize(static_cast<uint8_t>(LEAVE));
    remote.serialize(ip);
    remote.serialize(port);
}

void LeaveRequest::deserialize(RemoteConnection& remote)
{
    remote.deserialize(ip);
    remote.deserialize(port);
}

/* UpdateRequest */
void UpdateRequest::serialize(RemoteConnection& remote) const
{
    remote.serialize(static_cast<uint8_t>(UPDATE));
    remote.serialize(static_cast<uint8_t>(add ? 1 : 0));
    remote.serialize(static_cast<uint32_t>(members.size()));
    for (unsigned int i = 0; i < members.size(); i++) {
        remote.serialize(members[i].ip);
        remote.serialize(static_cast<uint32_t>(members[i].port));
    }
}

void UpdateRequest::deserialize(RemoteConnection& remote)
{
    uint8_t add_;
    uint32_t sz;
    string ip;
    uint32_t port;

    remote.deserialize(add_);
    add = add_;
    remote.deserialize(sz);
    for (unsigned int i = 0; i < sz; i++) {
        remote.deserialize(ip);
        remote.deserialize(port);
        members.push_back(Member(Remote(ip, port)));
    }
}

/* Response */
Response::Response(const string& _content) : content(_content)
{
}

void Response::serialize(RemoteConnection& remote) const
{
    remote.serialize(content);
}

void Response::deserialize(RemoteConnection& remote)
{
    remote.deserialize(content);
}
