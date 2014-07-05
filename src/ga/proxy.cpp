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
#include <signal.h>

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

/* Set to "true" by join() if the operation is successful. */
static bool joined = false;
static unsigned int server_port = ~0;

static int
join(unsigned int port)
{
    Remote remote("127.0.0.1", MANAGER_PORT);
    RemoteConnection connection(remote);
    JoinRequest message("127.0.0.1", port);
    Response response;

    message.serialize(connection);

    response.deserialize(connection);

    connection.close();

    if (response.content != "OK") {
        cout << "Join error: " << response.content << endl;
        exit(EXIT_FAILURE);
    }

    joined = true;
    server_port = port;
}

static int
leave(unsigned int port)
{
    Remote remote("127.0.0.1", MANAGER_PORT);
    RemoteConnection connection(remote);
    LeaveRequest request("127.0.0.1", port);
    Response response;

    request.serialize(connection);
    response.deserialize(connection);
    connection.close();

    if (response.content != "OK") {
        cout << "Leave error: " << response.content << endl;
        exit(EXIT_FAILURE);
    }

    joined = false;
}

static int
server(unsigned int port)
{
    MemberServer server(port);

    server.run();

    return 0;
}

static void
sigint_handler(int signum)
{
    if (joined) {
        leave(server_port);
    }

    exit(EXIT_SUCCESS);
}

int
main(int argc, char **argv)
{
    unsigned int port;
    struct sigaction sa;

    if (argc < 2) {
        exit_with_error("USAGE: program PORT");
    }
    port = atoi(argv[1]);
    if (port >= 65535) {
        errno = EINVAL;
        exit_with_error("PORT > 65535");
    }

    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        cout << "   Warning: SIGINT handler registration failed" << endl;
    }

    join(port);
    server(port);

    return 0;
}
