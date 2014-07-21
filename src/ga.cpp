#include "ga.h"
#include "remote.hpp"


GAPeerServer::GAPeerServer(unsigned int s_port, unsigned int j_port)
                    : PeerServer(s_port, j_port)
{
}

int GAPeerServer::process_message(uint8_t opcode, RemoteConnection& connection)
{
    return 0;
}

void MigrationMsg::serialize(RemoteConnection& remote) const
{
    remote.serialize(buf, len);
}

void MigrationMsg::deserialize(RemoteConnection& remote)
{
    unsigned int retlen;

    remote.deserialize(buf, len, retlen);
    if (retlen != len) {
        std::cerr << __func__ << "Migration problem" << endl;
    }
}
