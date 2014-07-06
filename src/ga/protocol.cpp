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
void UpdateRequest::add(const std::string& ip, uint32_t port)
{
    ips.push_back(ip);
    ports.push_back(port);
    num++;
}

void UpdateRequest::get(unsigned int idx, std::string& ip, uint32_t& port)
{
    if (idx >= size()) {
        return;
    }

    ip = ips[idx];
    port = ports[idx];
}

void UpdateRequest::serialize(RemoteConnection& remote) const
{
    remote.serialize(static_cast<uint8_t>(UPDATE));
    remote.serialize(num);
    for (vector<string>::const_iterator it = ips.begin();
                                        it != ips.end(); it++) {
        remote.serialize(*it);
    }
    for (vector<uint32_t>::const_iterator it = ports.begin();
                                    it != ports.end(); it++) {
        remote.serialize(*it);
    }
}

void UpdateRequest::deserialize(RemoteConnection& remote)
{
    remote.deserialize(num);
    for (uint32_t i = 0; i < num; i++) {
        string ip;

        remote.deserialize(ip);
        ips.push_back(ip);
    }
    for (uint32_t i = 0; i < num; i++) {
        uint32_t port;

        remote.deserialize(port);
        ports.push_back(port);
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
