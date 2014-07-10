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
#include <signal.h>
#include <cerrno>

using namespace std;


class ManagerServer : public Server {
        NodeColor next_color;
        unsigned int next_id;
        list<Member> members;

    public:
        ManagerServer(short unsigned port) : Server(port),
                            next_color(MPL_BLACK), next_id(1) { }

        virtual int process_request(RemoteConnection& connection);

        int add_member(const Member& remote);
        int del_member(const Remote& remote);
        void update_new_member(unsigned int new_id);
        void update_old_members(unsigned int new_id);
        void print_members();
};

void
ManagerServer::print_members()
{
    cout << "Members list" << endl;
    for (list<Member>::iterator it = members.begin();
            it != members.end(); it++) {
        cout << "    " << it->ip << ":" << it->port <<
                ", <" << it->id << ", " << it->color << ">" << endl;
    }
}

int
ManagerServer::add_member(const Member& remote)
{
    Member member(remote);

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (member == *it) {
            return -1;
        }
    }
    members.push_back(member);
    print_members();

    return 0;
}

int
ManagerServer::del_member(const Remote& remote)
{
    Member member(remote);

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (member == *it) {
            members.erase(it);
            print_members();

            return 0;
        }
    }

    return -1;
}

void
ManagerServer::update_new_member(unsigned int new_id)
{
    UpdateRequest request;
    list<Member>::iterator nit = members.end();

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (it->id != new_id) {
            request.members.push_back(*it);
        } else {
            nit = it;
        }
    }

    if (nit == members.end()) {
        cerr << __func__ << ": Internal error" << endl;
        exit(EXIT_FAILURE);
    }

    RemoteConnection connection(*nit);

    request.serialize(connection);
    connection.close();
}

void
ManagerServer::update_old_members(unsigned int new_id)
{
    list<Member>::iterator nit = members.end();

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (it->id == new_id) {
            nit = it;
        }
    }

    if (nit == members.end()) {
        cerr << __func__ << ": Internal error" << endl;
        exit(EXIT_FAILURE);
    }

    for (list<Member>::iterator it = members.begin();
                            it != members.end(); it++) {
        if (it != nit) {
            UpdateRequest request;
            RemoteConnection connection(*it);

            request.members.push_back(*nit);
            request.serialize(connection);
            connection.close();
        }
    }
}

int ManagerServer::process_request(RemoteConnection& connection)
{
    uint8_t opcode;

    cout << "Request received from : " << connection.remote.ip <<
        ":" << connection.remote.port << endl;
    connection.deserialize(opcode);

    if (opcode == JOIN) {
        string content = "OK";
        JoinRequest request;
        int ret;

        /* Process the join request. */
        request.deserialize(connection);
        cout << "JOIN-REQUEST(" << request.ip << "," << request.port
                << ")" << endl;

        Member member(Remote(request.ip, request.port));

        member.color = next_color;
        member.id = next_id;

        ret = add_member(member);
        if (ret < 0) {
            content = "Already joined";
        } else {
            if (next_color == MPL_BLACK) {
                next_color = MPL_RED;
            } else {
                next_color = MPL_BLACK;
            }
            next_id++;
        }

        Response(content).serialize(connection);

        update_new_member(member.id);
        update_old_members(member.id);

    } else if (opcode == LEAVE) {
        string content = "OK";
        LeaveRequest request;
        int ret;

        request.deserialize(connection);
        cout << "LEAVE-REQUEST(" << request.ip << "," << request.port
                << ")" << endl;
        ret = del_member(Remote(request.ip, request.port));
        if (ret) {
            content = "No previous join";
        }
        Response(content).serialize(connection);
    } else if (opcode == UPDATE) {
        UpdateRequest request;

        request.deserialize(connection);
        cout << "UPDATE-REQUEST-" << request.add << "-(" <<
                request.members.size() << ")" << endl;
        for (unsigned int i = 0; i < request.members.size(); i++) {
            const Member& m = request.members[i];
            cout << "   Member " << m.ip << " " << m.port << " "
                    << m.id << " " << m.color << endl;
        }
    }

    return 0;
}

/* Set to "true" by join() if the operation is successful. */
static bool joined = false;
static unsigned int server_port = ~0U;
static unsigned int join_port = ~0U;

static int
join()
{
    Remote remote("127.0.0.1", join_port);
    RemoteConnection connection(remote);
    JoinRequest message("127.0.0.1", server_port);
    Response response;

    message.serialize(connection);

    response.deserialize(connection);

    connection.close();

    if (response.content != "OK") {
        cout << "Join error: " << response.content << endl;
        exit(EXIT_FAILURE);
    }

    joined = true;

    return 0;
}

static int
leave()
{
    Remote remote("127.0.0.1", join_port);
    RemoteConnection connection(remote);
    LeaveRequest request("127.0.0.1", server_port);
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
    ManagerServer server(sargs->port);

    server.run();

    return NULL;
}

static void
sigint_handler(int signum)
{
    if (joined) {
        leave();
    }

    exit(EXIT_SUCCESS);
}

int
main(int argc, char **argv)
{
    unsigned int s_port;
    unsigned int j_port = ~0U;
    struct sigaction sa;
    pthread_t server_tid;
    ServerArgs sargs;

    if (argc < 2) {
        exit_with_error("USAGE: program PORT [JOINPORT]");
    }
    s_port = atoi(argv[1]);
    if (s_port >= 65535) {
        errno = EINVAL;
        exit_with_error("PORT > 65535");
    }
    server_port = s_port;

    if (argc > 2) {
        j_port = atoi(argv[2]);
        if (j_port >= 65535) {
            errno = EINVAL;
            exit_with_error("PORT > 65535");
        }
        join_port = j_port;
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

    /* Start the server. */
    sargs.port = s_port;
    if (pthread_create(&server_tid, NULL, server, (void *)&sargs)) {
        exit_with_error("pthread_create()");
    }

    if (j_port != ~0U) {
        /* Carry out the join procedure with the manager server. */
        join();
    }

    /* Wait for the member server to complete - it still holds the memory
     * for 'sargs', which is ours. */
    if (pthread_join(server_tid, NULL)) {
        exit_with_error("pthread_join()");
    }

    return 0;
}
