#include "common.hpp"
#include "remote.hpp"
#include "member.hpp"

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

using namespace std;


class MemberServer : public Server {
    public:
        MemberServer(short unsigned port) : Server(port) { }
        virtual int process_request(const RemoteConnection& connection);
};

int MemberServer::process_request(const RemoteConnection& connection)
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

int join()
{
        Remote remote("127.0.0.1", MANAGER_PORT);
        RemoteConnection connection(remote);
        const char *msg = "HELO";
        unsigned len = strlen(msg) + 1;
        int n;

        n = connection.send_message(msg, len);
        if (n != len) {
                exit_with_error("connection.send_message()");
        }

        //n = connection.recv_messages();

        connection.close();
}

int server(unsigned port)
{
        MemberServer server(port);

        server.run();

        return 0;
}

int main(int argc, char **argv)
{
        unsigned port;

        if (argc < 2) {
                exit_with_error("USAGE: program PORT");
        }
        port = atoi(argv[1]);

        join();
        server(port);

        return 0;
}
