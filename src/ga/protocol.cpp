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


JoinMessage::JoinMessage(const string& _ip, uint32_t _port) :
                                        ip(_ip), port(_port)
{
}

void JoinMessage::serialize(RemoteConnection& remote) const
{
        remote.serialize(ip);
        remote.serialize(port);
}

void JoinMessage::deserialize(RemoteConnection& remote)
{
        remote.deserialize(ip);
        remote.deserialize(port);
}

