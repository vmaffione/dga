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

    return 0;
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

    return 0;
}

struct ServerArgs {
    unsigned int port;
};

static void *
server(void *arg)
{
    ServerArgs *sargs = static_cast<ServerArgs*>(arg);
    MemberServer server(sargs->port);

    server.run();

    return NULL;
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
    pthread_t server_tid;
    ServerArgs sargs;

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
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        cout << "   Warning: SIGTERM handler registration failed" << endl;
    }

    /* Start the member server. */
    sargs.port = port;
    if (pthread_create(&server_tid, NULL, server, (void *)&sargs)) {
        exit_with_error("pthread_create()");
    }

    /* Carry out the join procedure with the manager server. */
    join(port);

    /* Wait for the member server to complete - it still holds the memory
     * for 'sargs', which is ours. */
    if (pthread_join(server_tid, NULL)) {
        exit_with_error("pthread_join()");
    }

    return 0;
}
