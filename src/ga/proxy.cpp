#include "remote.hpp"
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
#include <cerrno>

using namespace std;


class MemberServer : public Server {
    public:
        MemberServer(short unsigned port) : Server(port) { }
        virtual int process_request(RemoteConnection& connection);
};

int MemberServer::process_request(RemoteConnection& connection)
{
#define BUFSIZE 128
    char buffer[BUFSIZE];
    int n;

    cout << "Request received: " <<
        connection.remote.ip << ":"
        << connection.remote.port << "\n";

    n = connection.recv_message(buffer, sizeof(buffer));
    n = connection.send_message(buffer, n);

    return 0;
}

int join(unsigned int port)
{
    Remote remote("127.0.0.1", MANAGER_PORT);
    RemoteConnection connection(remote);
    JoinRequest message("127.0.0.1", port);
    Response response;

    message.serialize(connection);

    response.deserialize(connection);
    cout << "Response: " << response.content << "\n";

    connection.close();
}

int server(unsigned int port)
{
    MemberServer server(port);

    server.run();

    return 0;
}

int main(int argc, char **argv)
{
    unsigned int port;

    if (argc < 2) {
        exit_with_error("USAGE: program PORT");
    }
    port = atoi(argv[1]);
    if (port >= 65535) {
        errno = EINVAL;
        exit_with_error("PORT > 65535");
    }

    join(port);
    server(port);

    return 0;
}
