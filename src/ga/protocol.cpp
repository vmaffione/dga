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
#include <list>
#include <stdint.h>
#include <sstream>

using namespace std;


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
