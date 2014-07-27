#include "ifd.hpp"
#include "ga.hpp"
#include "remote.hpp"

#include <iostream>

using namespace std;


GAReceiveBuffer::GAReceiveBuffer(unsigned int sz) : size(sz)
{
    ptr = new uint8_t[size];
    len = 0;
    pthread_mutex_init(&lock_, NULL);
}

GAReceiveBuffer::~GAReceiveBuffer()
{
    pthread_mutex_destroy(&lock_);
    delete [] ptr;
}

void GAReceiveBuffer::lock()
{
    pthread_mutex_lock(&lock_);
}

void GAReceiveBuffer::unlock()
{
    pthread_mutex_unlock(&lock_);
}

GAPeerServer::GAPeerServer(const string& s_ip, unsigned int s_port,
                           const string& j_ip, unsigned int j_port)
                    : PeerServer(s_ip, s_port, j_ip, j_port), recvbuf(NULL)
{
}

void GAPeerServer::set_receive_buffer(GAReceiveBuffer *buf)
{
    recvbuf = buf;
}

int GAPeerServer::process_message(uint8_t opcode, RemoteConnection& connection)
{
    if (opcode != MIGRATE_MSG_ID) {
        cerr << __func__ << "Unknown opcode " << opcode << endl;
        return -1;
    }

    if (!recvbuf) {
        cerr << __func__ << "NULL receive buffer" << endl;
    }

    MigrationMsg msg(recvbuf->ptr, recvbuf->size);

    recvbuf->lock();
    recvbuf->len = msg.deserialize(connection);
    if (recvbuf->len != recvbuf->size) {
        cerr << __func__ << "Wrong message format: len = " <<
                recvbuf->len << ", expected len = " << recvbuf->size
                << endl;
        recvbuf->len = 0;
    }
    recvbuf->unlock();

#if (DBG >= DBG_LOT)
    cout << "Migration message received (" << recvbuf->len
            << " bytes)" << endl;
#endif

    return 0;
}

int MigrationMsg::serialize(RemoteConnection& remote) const
{
    remote.serialize(static_cast<uint8_t>(MIGRATE_MSG_ID));
    remote.serialize(buf, len);

    return 0;
}

int MigrationMsg::deserialize(RemoteConnection& remote)
{
    unsigned int retlen;

    remote.deserialize(buf, len, retlen);

    return retlen;
}
